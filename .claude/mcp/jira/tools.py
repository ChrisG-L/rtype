"""
Jira MCP Tools - Fonctions d'interaction avec l'API Jira.

Ce module contient les fonctions appelées par le serveur MCP pour
interagir avec Jira Cloud via l'API REST v3.
"""

from __future__ import annotations

import base64
import json
import logging
import re
import urllib.error
import urllib.request
from typing import Any, Optional
from dataclasses import dataclass

logger = logging.getLogger("jira.mcp.tools")


# =============================================================================
# CONFIGURATION
# =============================================================================

@dataclass
class JiraConfig:
    """Configuration de connexion Jira."""
    url: str
    email: str
    api_token: str

    @property
    def base_url(self) -> str:
        """URL de base pour l'API REST."""
        return f"{self.url.rstrip('/')}/rest/api/3"

    @property
    def auth_header(self) -> str:
        """Header d'authentification Basic."""
        credentials = f"{self.email}:{self.api_token}"
        encoded = base64.b64encode(credentials.encode()).decode()
        return f"Basic {encoded}"


# =============================================================================
# API CLIENT
# =============================================================================

class JiraClient:
    """Client HTTP minimaliste pour l'API Jira."""

    def __init__(self, config: JiraConfig) -> None:
        self.config = config

    def _request(
        self,
        method: str,
        endpoint: str,
        data: Optional[dict] = None,
        params: Optional[dict] = None
    ) -> dict[str, Any]:
        """
        Effectue une requête HTTP vers l'API Jira.

        Args:
            method: GET, POST, PUT, DELETE
            endpoint: Endpoint de l'API (ex: /issue/PROJ-123)
            data: Corps de la requête (pour POST/PUT)
            params: Paramètres de query string

        Returns:
            Réponse JSON parsée

        Raises:
            JiraError: En cas d'erreur API
        """
        url = f"{self.config.base_url}{endpoint}"

        if params:
            query_string = "&".join(f"{k}={urllib.parse.quote(str(v))}" for k, v in params.items())
            url = f"{url}?{query_string}"

        headers = {
            "Authorization": self.config.auth_header,
            "Content-Type": "application/json",
            "Accept": "application/json",
        }

        body = json.dumps(data).encode() if data else None

        request = urllib.request.Request(url, data=body, headers=headers, method=method)

        try:
            with urllib.request.urlopen(request, timeout=30) as response:
                response_body = response.read().decode()
                if response_body:
                    return json.loads(response_body)
                return {}
        except urllib.error.HTTPError as e:
            error_body = e.read().decode() if e.fp else ""
            logger.error(f"Jira API error: {e.code} - {error_body}")
            raise JiraError(e.code, error_body) from e
        except urllib.error.URLError as e:
            logger.error(f"Network error: {e.reason}")
            raise JiraError(0, str(e.reason)) from e

    def get(self, endpoint: str, params: Optional[dict] = None) -> dict[str, Any]:
        return self._request("GET", endpoint, params=params)

    def post(self, endpoint: str, data: dict) -> dict[str, Any]:
        return self._request("POST", endpoint, data=data)


class JiraError(Exception):
    """Erreur de l'API Jira."""

    def __init__(self, status_code: int, message: str) -> None:
        self.status_code = status_code
        self.message = message
        super().__init__(f"Jira API Error {status_code}: {message}")


# =============================================================================
# TICKET KEY EXTRACTION
# =============================================================================

# Pattern pour les clés de ticket Jira (PROJECT-123)
TICKET_PATTERN = re.compile(r'\b([A-Z][A-Z0-9]+-\d+)\b')


def extract_ticket_keys(text: str) -> list[str]:
    """
    Extrait les clés de tickets Jira d'un texte.

    Args:
        text: Texte à analyser (commit message, branche, etc.)

    Returns:
        Liste des clés trouvées (ex: ["PROJ-123", "FEAT-456"])

    Examples:
        >>> extract_ticket_keys("[PROJ-123] Fix bug in login")
        ["PROJ-123"]
        >>> extract_ticket_keys("feature/PROJ-123-add-feature")
        ["PROJ-123"]
        >>> extract_ticket_keys("PROJ-123, PROJ-456: Multiple fixes")
        ["PROJ-123", "PROJ-456"]
    """
    matches = TICKET_PATTERN.findall(text)
    # Dédupliquer tout en préservant l'ordre
    seen = set()
    result = []
    for match in matches:
        if match not in seen:
            seen.add(match)
            result.append(match)
    return result


# =============================================================================
# TOOL IMPLEMENTATIONS
# =============================================================================

def get_issue(client: JiraClient, issue_key: str, fields: Optional[list[str]] = None) -> dict[str, Any]:
    """
    Récupère un ticket Jira par sa clé.

    Args:
        client: Client Jira configuré
        issue_key: Clé du ticket (ex: PROJ-123)
        fields: Liste des champs à récupérer (défaut: tous les champs utiles)

    Returns:
        Informations du ticket formatées pour le contexte Claude
    """
    # Champs par défaut utiles pour l'analyse de code
    default_fields = [
        "summary",
        "description",
        "status",
        "issuetype",
        "priority",
        "assignee",
        "reporter",
        "labels",
        "components",
        "created",
        "updated",
        "resolution",
        "parent",          # Pour les sous-tâches
        "subtasks",
        "issuelinks",      # Liens vers d'autres tickets
        "customfield_*",   # Acceptance criteria souvent en custom field
    ]

    params = {}
    if fields:
        params["fields"] = ",".join(fields)

    try:
        raw_issue = client.get(f"/issue/{issue_key}", params=params if params else None)
        return _format_issue(raw_issue)
    except JiraError as e:
        if e.status_code == 404:
            return {"error": f"Issue {issue_key} not found"}
        raise


def _format_issue(raw: dict[str, Any]) -> dict[str, Any]:
    """Formate un ticket brut en structure propre pour Claude."""
    fields = raw.get("fields", {})

    # Extraire les infos de base
    result = {
        "key": raw.get("key"),
        "id": raw.get("id"),
        "url": raw.get("self", "").replace("/rest/api/3/issue/", "/browse/"),
        "summary": fields.get("summary"),
        "description": _extract_text(fields.get("description")),
        "status": _safe_get(fields, "status", "name"),
        "type": _safe_get(fields, "issuetype", "name"),
        "priority": _safe_get(fields, "priority", "name"),
        "assignee": _safe_get(fields, "assignee", "displayName"),
        "reporter": _safe_get(fields, "reporter", "displayName"),
        "labels": fields.get("labels", []),
        "components": [c.get("name") for c in fields.get("components", [])],
        "created": fields.get("created"),
        "updated": fields.get("updated"),
        "resolution": _safe_get(fields, "resolution", "name"),
    }

    # Parent (pour les sous-tâches)
    parent = fields.get("parent")
    if parent:
        result["parent"] = {
            "key": parent.get("key"),
            "summary": parent.get("fields", {}).get("summary"),
        }

    # Sous-tâches
    subtasks = fields.get("subtasks", [])
    if subtasks:
        result["subtasks"] = [
            {
                "key": st.get("key"),
                "summary": st.get("fields", {}).get("summary"),
                "status": _safe_get(st.get("fields", {}), "status", "name"),
            }
            for st in subtasks
        ]

    # Liens
    links = fields.get("issuelinks", [])
    if links:
        result["links"] = []
        for link in links:
            link_type = link.get("type", {}).get("name", "")
            if "inwardIssue" in link:
                linked = link["inwardIssue"]
                direction = link.get("type", {}).get("inward", link_type)
            else:
                linked = link.get("outwardIssue", {})
                direction = link.get("type", {}).get("outward", link_type)

            if linked:
                result["links"].append({
                    "type": direction,
                    "key": linked.get("key"),
                    "summary": linked.get("fields", {}).get("summary"),
                })

    # Chercher les acceptance criteria dans les custom fields
    for field_key, field_value in fields.items():
        if field_key.startswith("customfield_") and field_value:
            # Heuristique : si le nom contient "acceptance" ou "criteria"
            # On ne peut pas connaître le nom du champ sans les métadonnées
            # Mais on peut extraire le texte s'il y en a
            text = _extract_text(field_value)
            if text and len(text) > 50:  # Probablement du contenu utile
                if "acceptance_criteria" not in result:
                    result["acceptance_criteria"] = text

    return result


def _safe_get(obj: dict, *keys: str) -> Optional[str]:
    """Accès sécurisé à un champ imbriqué."""
    for key in keys:
        if obj is None:
            return None
        obj = obj.get(key) if isinstance(obj, dict) else None
    return obj


def _extract_text(content: Any) -> Optional[str]:
    """
    Extrait le texte d'un champ Jira (qui peut être en ADF format).

    Jira Cloud utilise Atlassian Document Format (ADF) pour les descriptions.
    Cette fonction convertit ADF en texte simple.
    """
    if content is None:
        return None

    if isinstance(content, str):
        return content

    if isinstance(content, dict):
        # Format ADF
        if content.get("type") == "doc":
            return _adf_to_text(content)
        # Ancien format ou autre
        return content.get("text", str(content))

    return str(content)


def _adf_to_text(node: dict) -> str:
    """Convertit un document ADF en texte simple."""
    if not isinstance(node, dict):
        return str(node) if node else ""

    node_type = node.get("type", "")
    text_parts = []

    # Texte direct
    if node_type == "text":
        return node.get("text", "")

    # Conteneur avec enfants
    content = node.get("content", [])
    for child in content:
        text_parts.append(_adf_to_text(child))

    result = "".join(text_parts)

    # Ajouter des séparateurs selon le type de bloc
    if node_type in ("paragraph", "heading", "bulletList", "orderedList"):
        result = result.strip() + "\n\n"
    elif node_type == "listItem":
        result = "• " + result.strip() + "\n"
    elif node_type == "codeBlock":
        result = f"```\n{result}\n```\n"

    return result


def search_issues(
    client: JiraClient,
    jql: str,
    max_results: int = 10,
    fields: Optional[list[str]] = None
) -> dict[str, Any]:
    """
    Recherche des tickets avec JQL.

    Args:
        client: Client Jira configuré
        jql: Requête JQL (ex: "project = PROJ AND status = Open")
        max_results: Nombre max de résultats (défaut: 10)
        fields: Champs à récupérer

    Returns:
        Liste des tickets trouvés
    """
    default_fields = ["summary", "status", "issuetype", "priority", "assignee", "updated"]

    data = {
        "jql": jql,
        "maxResults": max_results,
        "fields": fields or default_fields,
    }

    try:
        response = client.post("/search", data)
        issues = [_format_issue_summary(issue) for issue in response.get("issues", [])]
        return {
            "total": response.get("total", 0),
            "returned": len(issues),
            "issues": issues,
        }
    except JiraError as e:
        return {"error": str(e), "issues": []}


def _format_issue_summary(raw: dict[str, Any]) -> dict[str, Any]:
    """Formate un ticket en résumé léger."""
    fields = raw.get("fields", {})
    return {
        "key": raw.get("key"),
        "summary": fields.get("summary"),
        "status": _safe_get(fields, "status", "name"),
        "type": _safe_get(fields, "issuetype", "name"),
        "priority": _safe_get(fields, "priority", "name"),
        "assignee": _safe_get(fields, "assignee", "displayName"),
        "updated": fields.get("updated"),
    }


def get_issue_from_text(client: JiraClient, text: str) -> dict[str, Any]:
    """
    Extrait les clés de tickets d'un texte et récupère leurs infos.

    Utile pour extraire le contexte d'un commit message ou nom de branche.

    Args:
        client: Client Jira configuré
        text: Texte contenant potentiellement des clés de tickets

    Returns:
        Liste des tickets trouvés avec leurs infos
    """
    keys = extract_ticket_keys(text)

    if not keys:
        return {
            "source_text": text,
            "keys_found": [],
            "issues": [],
        }

    issues = []
    errors = []

    for key in keys:
        try:
            issue = get_issue(client, key)
            if "error" not in issue:
                issues.append(issue)
            else:
                errors.append({"key": key, "error": issue["error"]})
        except JiraError as e:
            errors.append({"key": key, "error": str(e)})

    return {
        "source_text": text,
        "keys_found": keys,
        "issues": issues,
        "errors": errors if errors else None,
    }


def get_project_info(client: JiraClient, project_key: str) -> dict[str, Any]:
    """
    Récupère les infos d'un projet Jira.

    Args:
        client: Client Jira configuré
        project_key: Clé du projet (ex: PROJ)

    Returns:
        Informations du projet
    """
    try:
        raw = client.get(f"/project/{project_key}")
        return {
            "key": raw.get("key"),
            "name": raw.get("name"),
            "description": raw.get("description"),
            "lead": _safe_get(raw, "lead", "displayName"),
            "url": raw.get("self", "").replace("/rest/api/3/project/", "/browse/"),
            "issue_types": [it.get("name") for it in raw.get("issueTypes", [])],
        }
    except JiraError as e:
        if e.status_code == 404:
            return {"error": f"Project {project_key} not found"}
        raise
