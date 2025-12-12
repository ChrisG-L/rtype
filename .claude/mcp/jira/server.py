"""
Jira MCP Server - Serveur MCP pour l'intégration Jira Cloud.

Ce module implémente le serveur MCP qui expose Jira aux agents Claude.

Architecture :
- Transport : stdio (JSON-RPC 2.0)
- Protocole : MCP (Model Context Protocol)
- Backend : API REST Jira v3

Le serveur expose 4 outils :
1. get_issue : Récupérer un ticket par sa clé
2. search_issues : Rechercher avec JQL
3. get_issue_from_text : Extraire et récupérer des tickets depuis un texte
4. get_project_info : Infos sur un projet

Usage:
    python -m mcp.jira.server

Configuration:
    Variables d'environnement :
    - JIRA_URL : https://company.atlassian.net
    - JIRA_EMAIL : user@company.com
    - JIRA_API_TOKEN : votre-token-api

    Ou .claude/settings.local.json :
    {
        "jira": {
            "url": "https://company.atlassian.net",
            "email": "user@company.com",
            "api_token": "votre-token"
        }
    }
"""

from __future__ import annotations

import asyncio
import json
import logging
import os
import sys
from pathlib import Path
from typing import Any, Callable, Optional

# Configuration du logging
logging.basicConfig(
    level=os.environ.get("JIRA_LOG_LEVEL", "INFO"),
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    stream=sys.stderr
)
logger = logging.getLogger("jira.mcp.server")


# =============================================================================
# CONSTANTS
# =============================================================================

SERVER_VERSION = "1.0.0"
SERVER_NAME = "jira"

# Codes d'erreur JSON-RPC
PARSE_ERROR = -32700
INVALID_REQUEST = -32600
METHOD_NOT_FOUND = -32601
INVALID_PARAMS = -32602
INTERNAL_ERROR = -32603

# Erreurs spécifiques Jira
JIRA_NOT_CONFIGURED = -32000
JIRA_API_ERROR = -32001


# =============================================================================
# TOOL DEFINITIONS
# =============================================================================

TOOL_DEFINITIONS = [
    {
        "name": "get_issue",
        "description": """Récupère un ticket Jira par sa clé.

Retourne toutes les informations utiles du ticket :
- summary, description, status, priority
- assignee, reporter, labels, components
- parent (si sous-tâche), subtasks, liens
- acceptance criteria (si disponible)

Exemple:
{
  "key": "PROJ-123",
  "summary": "Implémenter la fonctionnalité X",
  "description": "Description détaillée...",
  "status": "In Progress",
  "type": "Story",
  "priority": "High",
  "assignee": "John Doe",
  "labels": ["backend", "api"],
  "acceptance_criteria": "- [ ] Test A\\n- [ ] Test B"
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "issue_key": {
                    "type": "string",
                    "description": "Clé du ticket (ex: PROJ-123)"
                }
            },
            "required": ["issue_key"]
        }
    },
    {
        "name": "search_issues",
        "description": """Recherche des tickets Jira avec JQL.

JQL (Jira Query Language) permet des recherches puissantes :
- project = PROJ AND status = Open
- assignee = currentUser() AND updated >= -7d
- labels in (bug, critical) ORDER BY priority DESC

Retourne une liste de tickets avec leurs infos essentielles.

Exemple:
{
  "total": 42,
  "returned": 10,
  "issues": [
    {"key": "PROJ-123", "summary": "...", "status": "Open", ...},
    ...
  ]
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "jql": {
                    "type": "string",
                    "description": "Requête JQL"
                },
                "max_results": {
                    "type": "integer",
                    "default": 10,
                    "minimum": 1,
                    "maximum": 50,
                    "description": "Nombre max de résultats"
                }
            },
            "required": ["jql"]
        }
    },
    {
        "name": "get_issue_from_text",
        "description": """Extrait les clés de tickets d'un texte et récupère leurs infos.

Parfait pour extraire le contexte Jira d'un commit message ou nom de branche.
Détecte automatiquement les patterns comme PROJ-123.

Exemples de textes reconnus :
- "[PROJ-123] Fix bug in login"
- "feature/PROJ-123-add-feature"
- "PROJ-123, PROJ-456: Multiple fixes"

Retourne les infos complètes de chaque ticket trouvé.

Exemple:
{
  "source_text": "[PROJ-123] Fix login bug",
  "keys_found": ["PROJ-123"],
  "issues": [
    {"key": "PROJ-123", "summary": "Login bug", ...}
  ]
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "text": {
                    "type": "string",
                    "description": "Texte contenant potentiellement des clés de tickets (commit message, branche, etc.)"
                }
            },
            "required": ["text"]
        }
    },
    {
        "name": "get_project_info",
        "description": """Récupère les informations d'un projet Jira.

Utile pour connaître le contexte d'un projet :
- Nom et description
- Lead du projet
- Types d'issues disponibles

Exemple:
{
  "key": "PROJ",
  "name": "Mon Projet",
  "description": "Description du projet",
  "lead": "John Doe",
  "issue_types": ["Bug", "Story", "Task", "Epic"]
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "project_key": {
                    "type": "string",
                    "description": "Clé du projet (ex: PROJ)"
                }
            },
            "required": ["project_key"]
        }
    }
]


# =============================================================================
# CONFIGURATION LOADING
# =============================================================================

def load_jira_config() -> Optional[dict[str, str]]:
    """
    Charge la configuration Jira depuis l'environnement ou le fichier settings.

    Priorité :
    1. Variables d'environnement (JIRA_URL, JIRA_EMAIL, JIRA_API_TOKEN)
    2. .claude/settings.local.json
    3. .claude/settings.json

    Returns:
        Dict avec url, email, api_token ou None si non configuré
    """
    # 1. Variables d'environnement
    url = os.environ.get("JIRA_URL")
    email = os.environ.get("JIRA_EMAIL")
    token = os.environ.get("JIRA_API_TOKEN")

    if url and email and token:
        logger.info("Jira config loaded from environment variables")
        return {"url": url, "email": email, "api_token": token}

    # 2. Fichiers de settings
    for settings_file in [".claude/settings.local.json", ".claude/settings.json"]:
        settings_path = Path(settings_file)
        if settings_path.exists():
            try:
                with open(settings_path) as f:
                    settings = json.load(f)
                    jira_config = settings.get("jira", {})
                    if all(k in jira_config for k in ("url", "email", "api_token")):
                        logger.info(f"Jira config loaded from {settings_file}")
                        return jira_config
            except (json.JSONDecodeError, IOError) as e:
                logger.warning(f"Could not read {settings_file}: {e}")

    logger.warning("Jira not configured - tools will return errors")
    return None


# =============================================================================
# MCP SERVER CLASS
# =============================================================================

class JiraMCPServer:
    """
    Serveur MCP pour Jira.

    Expose les fonctionnalités Jira aux agents Claude via transport stdio.
    """

    def __init__(self) -> None:
        self.client = None
        self._initialized = False
        self.tool_handlers: dict[str, Callable] = {}

    def initialize(self) -> None:
        """Initialise la connexion Jira."""
        if self._initialized:
            return

        logger.info("Initializing Jira MCP server...")

        # Charger la config
        config_dict = load_jira_config()

        if config_dict:
            from .tools import JiraConfig, JiraClient
            config = JiraConfig(
                url=config_dict["url"],
                email=config_dict["email"],
                api_token=config_dict["api_token"]
            )
            self.client = JiraClient(config)
            logger.info(f"Connected to Jira: {config.url}")
        else:
            logger.warning("Jira not configured - server will start but tools will fail")

        # Enregistrer les handlers
        self._register_handlers()
        self._initialized = True
        logger.info("Jira MCP server initialized")

    def _register_handlers(self) -> None:
        """Enregistre les handlers pour chaque outil."""
        self.tool_handlers = {
            "get_issue": self._handle_get_issue,
            "search_issues": self._handle_search_issues,
            "get_issue_from_text": self._handle_get_issue_from_text,
            "get_project_info": self._handle_get_project_info,
        }

    def _check_client(self) -> None:
        """Vérifie que le client est configuré."""
        if self.client is None:
            raise ValueError(
                "Jira not configured. Set JIRA_URL, JIRA_EMAIL, JIRA_API_TOKEN "
                "or add jira config to .claude/settings.local.json"
            )

    # =========================================================================
    # TOOL HANDLERS
    # =========================================================================

    async def _handle_get_issue(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour get_issue."""
        self._check_client()
        from .tools import get_issue
        return get_issue(self.client, arguments["issue_key"])

    async def _handle_search_issues(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour search_issues."""
        self._check_client()
        from .tools import search_issues
        return search_issues(
            self.client,
            jql=arguments["jql"],
            max_results=arguments.get("max_results", 10)
        )

    async def _handle_get_issue_from_text(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour get_issue_from_text."""
        self._check_client()
        from .tools import get_issue_from_text
        return get_issue_from_text(self.client, arguments["text"])

    async def _handle_get_project_info(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour get_project_info."""
        self._check_client()
        from .tools import get_project_info
        return get_project_info(self.client, arguments["project_key"])

    # =========================================================================
    # MCP PROTOCOL
    # =========================================================================

    async def handle_request(self, request: dict[str, Any]) -> dict[str, Any]:
        """Traite une requête JSON-RPC 2.0."""
        request_id = request.get("id")
        method = request.get("method", "")
        params = request.get("params", {})

        logger.debug(f"Handling request: method={method}, id={request_id}")

        try:
            if method == "initialize":
                result = await self._handle_initialize(params)
            elif method == "initialized":
                result = {}
            elif method == "tools/list":
                result = await self._handle_tools_list(params)
            elif method == "tools/call":
                result = await self._handle_tools_call(params)
            elif method == "shutdown":
                result = {}
            else:
                return self._error_response(request_id, METHOD_NOT_FOUND, f"Unknown method: {method}")

            return self._success_response(request_id, result)

        except Exception as e:
            logger.error(f"Error handling request: {e}", exc_info=True)
            return self._error_response(request_id, INTERNAL_ERROR, str(e))

    async def _handle_initialize(self, params: dict[str, Any]) -> dict[str, Any]:
        """Handle initialize request."""
        return {
            "protocolVersion": "2024-11-05",
            "serverInfo": {
                "name": SERVER_NAME,
                "version": SERVER_VERSION,
            },
            "capabilities": {
                "tools": {"listChanged": False}
            }
        }

    async def _handle_tools_list(self, params: dict[str, Any]) -> dict[str, Any]:
        """Handle tools/list request."""
        return {"tools": TOOL_DEFINITIONS}

    async def _handle_tools_call(self, params: dict[str, Any]) -> dict[str, Any]:
        """Handle tools/call request."""
        tool_name = params.get("name")
        arguments = params.get("arguments", {})

        if tool_name not in self.tool_handlers:
            raise ValueError(f"Unknown tool: {tool_name}")

        handler = self.tool_handlers[tool_name]
        result = await handler(arguments)

        return {
            "content": [
                {
                    "type": "text",
                    "text": json.dumps(result, indent=2, ensure_ascii=False)
                }
            ]
        }

    def _success_response(self, request_id: Any, result: Any) -> dict[str, Any]:
        """Build a success response."""
        return {
            "jsonrpc": "2.0",
            "id": request_id,
            "result": result
        }

    def _error_response(self, request_id: Any, code: int, message: str) -> dict[str, Any]:
        """Build an error response."""
        return {
            "jsonrpc": "2.0",
            "id": request_id,
            "error": {"code": code, "message": message}
        }

    # =========================================================================
    # STDIO TRANSPORT
    # =========================================================================

    async def run(self) -> None:
        """Lance le serveur en mode stdio."""
        logger.info("Starting Jira MCP server (stdio transport)...")

        self.initialize()

        reader = asyncio.StreamReader()
        protocol = asyncio.StreamReaderProtocol(reader)
        await asyncio.get_event_loop().connect_read_pipe(lambda: protocol, sys.stdin)

        writer_transport, writer_protocol = await asyncio.get_event_loop().connect_write_pipe(
            asyncio.streams.FlowControlMixin, sys.stdout
        )
        writer = asyncio.StreamWriter(writer_transport, writer_protocol, reader, asyncio.get_event_loop())

        logger.info("Server ready, waiting for requests...")

        try:
            while True:
                line = await reader.readline()
                if not line:
                    logger.info("EOF received, shutting down")
                    break

                line = line.decode("utf-8").strip()
                if not line:
                    continue

                logger.debug(f"Received: {line[:200]}...")

                try:
                    request = json.loads(line)
                    response = await self.handle_request(request)

                    if response:
                        response_str = json.dumps(response, ensure_ascii=False) + "\n"
                        writer.write(response_str.encode("utf-8"))
                        await writer.drain()
                        logger.debug(f"Sent: {response_str[:200]}...")

                except json.JSONDecodeError as e:
                    error_response = self._error_response(None, PARSE_ERROR, f"Invalid JSON: {e}")
                    writer.write((json.dumps(error_response) + "\n").encode("utf-8"))
                    await writer.drain()

        except KeyboardInterrupt:
            logger.info("Server stopped by user")
        except Exception as e:
            logger.error(f"Server error: {e}", exc_info=True)
            raise


# =============================================================================
# ENTRY POINT
# =============================================================================

def main() -> None:
    """Point d'entrée du serveur MCP."""
    server = JiraMCPServer()
    asyncio.run(server.run())


if __name__ == "__main__":
    main()
