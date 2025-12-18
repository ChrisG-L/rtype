#!/usr/bin/env python3
"""
Transform SonarQube JSON report to Markdown summary for SYNTHESIS agent.

This script reads SonarQube API output (/api/issues/search) and produces:
1. A compact, actionable Markdown report suitable for AI agents (sonar.md)
2. A JSON file with issues in where/why/how format for web display (sonar-issues.json)

Usage:
    python transform-sonar.py issues.json
    python transform-sonar.py issues.json --output report.md --commit abc123 --branch main
    python transform-sonar.py issues.json --db .claude/agentdb/db.sqlite
"""

import argparse
import json
import re
import sqlite3
import sys
from collections import defaultdict
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any


# =============================================================================
# MAPPINGS - SonarQube values → Web TypeScript types
# =============================================================================

# Case-insensitive mapping for all SonarQube severity variants
# API uses BLOCKER/CRITICAL/MAJOR/MINOR/INFO
# UI sometimes uses Blocker/High/Medium/Low/Info
SEVERITY_MAP = {
    # API format (uppercase)
    "BLOCKER": "Blocker",
    "CRITICAL": "Critical",
    "MAJOR": "Major",
    "MINOR": "Minor",
    "INFO": "Info",
    # UI format variants
    "HIGH": "Critical",
    "MEDIUM": "Major",
    "LOW": "Minor",
}

def normalize_severity(raw_severity: str) -> str:
    """Normalize severity to web TypeScript format, case-insensitive."""
    if not raw_severity:
        return "Info"
    upper = raw_severity.upper()
    return SEVERITY_MAP.get(upper, "Info")

# Ordered for display (most severe first)
SEVERITY_ORDER = ["Blocker", "Critical", "Major", "Minor", "Info"]

TYPE_TO_CATEGORY_MAP = {
    "VULNERABILITY": "Security",
    "BUG": "Reliability",
    "CODE_SMELL": "Maintainability",
}

CATEGORY_ORDER = ["Security", "Reliability", "Maintainability"]

# Status values to include (ignore RESOLVED, CLOSED, etc.)
ACTIVE_STATUSES = {"OPEN", "CONFIRMED", "REOPENED"}

# Default time filter for issues (48 hours)
DEFAULT_SINCE_HOURS = 48


# =============================================================================
# DATE FILTERING
# =============================================================================

def parse_since_argument(since_str: str | None, no_filter: bool = False) -> datetime | None:
    """Parse --since argument into a datetime.

    Supports:
    - Relative durations: "24h", "48h", "7d", "2w"
    - ISO dates: "2025-12-10", "2025-12-10T14:30:00"
    - "none" or "all" to disable filtering

    Returns None if no filtering should be applied.
    """
    if no_filter:
        return None

    if not since_str:
        # Default: 48 hours
        return datetime.now(timezone.utc) - timedelta(hours=DEFAULT_SINCE_HOURS)

    # Check for explicit "no filter" values
    if since_str.lower() in ("none", "all", "0"):
        return None

    # Try relative duration patterns
    duration_pattern = re.match(r'^(\d+)([hdwm])$', since_str.lower())
    if duration_pattern:
        value = int(duration_pattern.group(1))
        unit = duration_pattern.group(2)

        now = datetime.now(timezone.utc)
        if unit == 'h':
            return now - timedelta(hours=value)
        elif unit == 'd':
            return now - timedelta(days=value)
        elif unit == 'w':
            return now - timedelta(weeks=value)
        elif unit == 'm':
            return now - timedelta(days=value * 30)  # Approximate months

    # Try ISO date formats
    for fmt in [
        "%Y-%m-%dT%H:%M:%S%z",      # 2025-12-10T14:30:00+0000
        "%Y-%m-%dT%H:%M:%S",        # 2025-12-10T14:30:00
        "%Y-%m-%d",                 # 2025-12-10
    ]:
        try:
            parsed = datetime.strptime(since_str, fmt)
            if parsed.tzinfo is None:
                parsed = parsed.replace(tzinfo=timezone.utc)
            return parsed
        except ValueError:
            continue

    print(f"⚠️  Invalid --since format: {since_str}, using default (48h)", file=sys.stderr)
    return datetime.now(timezone.utc) - timedelta(hours=DEFAULT_SINCE_HOURS)


def parse_sonarqube_date(date_str: str) -> datetime | None:
    """Parse SonarQube creationDate format.

    SonarQube uses format: "2025-12-12T10:30:00+0000"
    """
    if not date_str:
        return None

    # SonarQube format: "2025-12-12T10:30:00+0000"
    for fmt in [
        "%Y-%m-%dT%H:%M:%S%z",
        "%Y-%m-%dT%H:%M:%S.%f%z",
    ]:
        try:
            return datetime.strptime(date_str, fmt)
        except ValueError:
            continue

    return None


def is_issue_recent(raw_issue: dict, since_date: datetime | None) -> bool:
    """Check if issue was created after the since_date."""
    if since_date is None:
        return True

    creation_date_str = raw_issue.get("creationDate")
    if not creation_date_str:
        # If no creation date, include the issue (conservative approach)
        return True

    creation_date = parse_sonarqube_date(creation_date_str)
    if creation_date is None:
        return True

    return creation_date >= since_date


# =============================================================================
# DATA CLASSES
# =============================================================================

class Issue:
    """Represents a single SonarQube issue with mapped values."""

    def __init__(self, raw: dict[str, Any], rules_map: dict[str, str]):
        self.message = raw.get("message", "No message")
        self.severity = normalize_severity(raw.get("severity", "INFO"))
        self.category = TYPE_TO_CATEGORY_MAP.get(raw.get("type", "CODE_SMELL"), "Maintainability")
        self.rule = raw.get("rule", "unknown")
        self.rule_name = rules_map.get(self.rule, self.rule)
        self.effort = raw.get("effort", "unknown")
        self.status = raw.get("status", "OPEN")

        # Extract file path from component (remove "project:" prefix)
        component = raw.get("component", "")
        if ":" in component:
            self.file = component.split(":", 1)[1]
        else:
            self.file = component

        self.line = raw.get("line", 0)

    @property
    def location(self) -> str:
        """Return file:line format."""
        if self.line:
            return f"{self.file}:{self.line}"
        return self.file

    def is_active(self) -> bool:
        """Check if issue should be included in report."""
        return self.status in ACTIVE_STATUSES


class SonarReport:
    """Aggregated SonarQube report data."""

    def __init__(self):
        self.issues: list[Issue] = []
        self.by_severity: dict[str, list[Issue]] = defaultdict(list)
        self.by_category: dict[str, list[Issue]] = defaultdict(list)
        self.by_file: dict[str, dict[str, int]] = defaultdict(lambda: defaultdict(int))
        self.by_rule: dict[str, dict[str, Any]] = defaultdict(lambda: {"name": "", "count": 0})
        self.total = 0

    def add_issue(self, issue: Issue):
        """Add an issue and update all aggregations."""
        self.issues.append(issue)
        self.by_severity[issue.severity].append(issue)
        self.by_category[issue.category].append(issue)
        self.by_file[issue.file][issue.severity] += 1
        self.by_rule[issue.rule]["name"] = issue.rule_name
        self.by_rule[issue.rule]["count"] += 1
        self.total += 1

    def severity_count(self, severity: str) -> int:
        """Get count for a specific severity."""
        return len(self.by_severity.get(severity, []))

    def category_count(self, category: str) -> int:
        """Get count for a specific category."""
        return len(self.by_category.get(category, []))


# =============================================================================
# PARSING
# =============================================================================

def normalize_path(path: str) -> str:
    """Normalize file path for comparison (remove leading ./ and trailing whitespace)."""
    path = path.strip()
    if path.startswith("./"):
        path = path[2:]
    return path


def path_matches(issue_path: str, filter_paths: set[str]) -> bool:
    """Check if issue path matches any of the filter paths."""
    normalized_issue = normalize_path(issue_path)
    for filter_path in filter_paths:
        normalized_filter = normalize_path(filter_path)
        # Exact match or issue path ends with filter path (handles prefix differences)
        if normalized_issue == normalized_filter or normalized_issue.endswith("/" + normalized_filter):
            return True
        # Filter path ends with issue path (handles case where filter has more context)
        if normalized_filter.endswith("/" + normalized_issue) or normalized_filter == normalized_issue:
            return True
    return False


def parse_sonar_json(
    json_path: Path,
    file_filter: set[str] | None = None,
    since_date: datetime | None = None
) -> tuple[SonarReport, int, int]:
    """Parse SonarQube JSON and return aggregated report.

    Args:
        json_path: Path to SonarQube JSON file
        file_filter: Optional set of file paths to filter issues (for diff mode)
        since_date: Optional datetime to filter issues by creation date

    Returns:
        Tuple of (report, total_before_filter, filtered_by_date) where:
        - total_before_filter is the count of all active issues before file filtering
        - filtered_by_date is the count of issues excluded by date filter
    """

    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    # Build rules lookup map
    rules_map = {}
    for rule in data.get("rules", []):
        rules_map[rule.get("key", "")] = rule.get("name", "")

    report = SonarReport()
    total_before_filter = 0
    filtered_by_date = 0

    for raw_issue in data.get("issues", []):
        issue = Issue(raw_issue, rules_map)

        if not issue.is_active():
            continue

        total_before_filter += 1

        # Apply date filter if specified
        if since_date is not None and not is_issue_recent(raw_issue, since_date):
            filtered_by_date += 1
            continue

        # Apply file filter if specified
        if file_filter is not None:
            if not path_matches(issue.file, file_filter):
                continue

        report.add_issue(issue)

    return report, total_before_filter, filtered_by_date


# =============================================================================
# MARKDOWN GENERATION
# =============================================================================

def generate_markdown(report: SonarReport, commit: str | None, branch: str | None,
                      input_file: str, top_n: int,
                      file_filter: set[str] | None = None,
                      total_before_filter: int = 0) -> str:
    """Generate Markdown report from aggregated data."""

    lines = []

    # Header
    lines.append("# Rapport SonarQube - Résumé")
    lines.append("")

    # Metadata
    commit_str = commit if commit else "(non spécifié)"
    branch_str = branch if branch else "(non spécifiée)"
    date_str = datetime.now().strftime("%Y-%m-%d %H:%M")
    lines.append(f"Commit: {commit_str} | Branche: {branch_str} | Date: {date_str}")

    # Filtered mode indication
    if file_filter:
        lines.append("")
        lines.append(f"Mode: **Filtré sur {len(file_filter)} fichiers du diff**")
        lines.append("")
        lines.append("Fichiers analysés:")
        for fp in sorted(file_filter):
            lines.append(f"- {fp}")
        if total_before_filter > report.total:
            lines.append("")
            lines.append(f"*({report.total} issues sur les fichiers du diff, {total_before_filter} sur tout le projet)*")

    lines.append("")
    lines.append("---")
    lines.append("")

    # Overview section
    lines.append("## Vue d'ensemble")
    lines.append("")

    # By severity table
    lines.append("### Par sévérité")
    lines.append("")
    lines.append("| Sévérité | Count |")
    lines.append("|----------|-------|")
    for sev in SEVERITY_ORDER:
        count = report.severity_count(sev)
        lines.append(f"| {sev} | {count} |")
    lines.append(f"| **Total** | **{report.total}** |")
    lines.append("")

    # By category table
    lines.append("### Par catégorie")
    lines.append("")
    lines.append("| Catégorie | Count |")
    lines.append("|-----------|-------|")
    for cat in CATEGORY_ORDER:
        count = report.category_count(cat)
        lines.append(f"| {cat} | {count} |")
    lines.append("")
    lines.append("---")
    lines.append("")

    # Blocker issues (full detail)
    blockers = report.by_severity.get("Blocker", [])
    lines.append(f"## Issues Bloquantes ({len(blockers)})")
    lines.append("")
    if blockers:
        for i, issue in enumerate(blockers, 1):
            lines.extend(_format_issue_detail(i, issue))
    else:
        lines.append("Aucune issue bloquante.")
        lines.append("")
    lines.append("---")
    lines.append("")

    # Critical issues (full detail)
    criticals = report.by_severity.get("Critical", [])
    lines.append(f"## Issues Critiques ({len(criticals)})")
    lines.append("")
    if criticals:
        for i, issue in enumerate(criticals, 1):
            lines.extend(_format_issue_detail(i, issue))
    else:
        lines.append("Aucune issue critique.")
        lines.append("")
    lines.append("---")
    lines.append("")

    # Major issues (full detail, limited to 50)
    majors = report.by_severity.get("Major", [])
    lines.append(f"## Issues Majeures ({len(majors)})")
    lines.append("")
    if majors:
        display_majors = majors[:50]
        for i, issue in enumerate(display_majors, 1):
            lines.extend(_format_issue_detail(i, issue))
        if len(majors) > 50:
            lines.append(f"*... et {len(majors) - 50} autres issues majeures non listées.*")
            lines.append("")
    else:
        lines.append("Aucune issue majeure.")
        lines.append("")
    lines.append("---")
    lines.append("")

    # Top impacted files
    lines.append(f"## Fichiers les plus impactés (top {top_n})")
    lines.append("")
    lines.append("| Fichier | Blocker | Critical | Major | Minor | Total |")
    lines.append("|---------|---------|----------|-------|-------|-------|")

    # Sort files by total issues
    file_totals = []
    for file_path, counts in report.by_file.items():
        total = sum(counts.values())
        file_totals.append((file_path, counts, total))
    file_totals.sort(key=lambda x: x[2], reverse=True)

    for file_path, counts, total in file_totals[:top_n]:
        b = counts.get("Blocker", 0)
        c = counts.get("Critical", 0)
        m = counts.get("Major", 0)
        mi = counts.get("Minor", 0)
        lines.append(f"| {file_path} | {b} | {c} | {m} | {mi} | {total} |")

    if not file_totals:
        lines.append("| (aucun fichier) | 0 | 0 | 0 | 0 | 0 |")

    lines.append("")
    lines.append("---")
    lines.append("")

    # Top violated rules
    lines.append(f"## Règles les plus violées (top {top_n})")
    lines.append("")
    lines.append("| Règle | Description | Count |")
    lines.append("|-------|-------------|-------|")

    # Sort rules by count
    rule_items = [(rule, data["name"], data["count"]) for rule, data in report.by_rule.items()]
    rule_items.sort(key=lambda x: x[2], reverse=True)

    for rule, name, count in rule_items[:top_n]:
        # Truncate long rule names
        display_name = name[:50] + "..." if len(name) > 50 else name
        lines.append(f"| {rule} | {display_name} | {count} |")

    if not rule_items:
        lines.append("| (aucune règle) | - | 0 |")

    lines.append("")
    lines.append("---")
    lines.append("")

    # Reference to full file
    lines.append("## Fichier complet")
    lines.append("")
    if file_filter and total_before_filter > report.total:
        lines.append(f"Détails complets disponibles dans : `{input_file}` ({total_before_filter} issues sur tout le projet, {report.total} après filtrage)")
    else:
        lines.append(f"Détails complets disponibles dans : `{input_file}`")
    lines.append("")

    return "\n".join(lines)


def _format_issue_detail(index: int, issue: Issue) -> list[str]:
    """Format a single issue with full details."""
    lines = []
    lines.append(f"### {index}. {issue.message}")
    lines.append("")
    lines.append(f"- **Fichier**: {issue.location}")
    lines.append(f"- **Règle**: {issue.rule}")
    lines.append(f"- **Sévérité**: {issue.severity}")
    lines.append(f"- **Catégorie**: {issue.category}")
    lines.append(f"- **Effort**: {issue.effort}")
    lines.append("")
    return lines


# =============================================================================
# JSON GENERATION (for web-synthesizer)
# =============================================================================

# =============================================================================
# SONARQUBE RULE MAPPINGS
# =============================================================================

# Mapping des règles SonarQube spécifiques vers des suggestions de correction
RULE_SUGGESTIONS = {
    # Complexité et imbrication
    "S134": [
        "Extraire les blocs imbriqués dans des fonctions dédiées",
        "Utiliser des early returns pour réduire l'imbrication",
        "Appliquer le pattern Guard Clause"
    ],
    "S3776": [
        "Découper la fonction en sous-fonctions plus simples",
        "Réduire le nombre de branches conditionnelles",
        "Simplifier la logique en extrayant des méthodes"
    ],
    # Destructeurs et exceptions
    "S1048": [
        "Encapsuler le code du destructeur dans un try/catch",
        "Logger l'erreur au lieu de la propager",
        "Marquer le destructeur avec noexcept"
    ],
    # Types et pointeurs
    "S5008": [
        "Utiliser des templates pour la généricité typée",
        "Utiliser std::any ou std::variant selon le cas",
        "Préférer les types concrets aux void*"
    ],
    # Blocs vides
    "S1186": [
        "Implémenter la logique manquante",
        "Ajouter un commentaire expliquant pourquoi le bloc est vide",
        "Supprimer le bloc s'il n'est pas nécessaire"
    ],
    "S108": [
        "Implémenter le code manquant",
        "Ajouter un commentaire justifiant le bloc vide",
        "Supprimer le bloc vide si inutile"
    ],
    # Exceptions
    "S112": [
        "Créer une exception dédiée héritant de std::runtime_error",
        "Utiliser std::system_error pour les erreurs système",
        "Documenter les exceptions dans la signature"
    ],
    # Lambdas
    "S3608": [
        "Capturer explicitement les variables nécessaires",
        "Lister les variables au lieu de [=] ou [&]",
        "Évaluer si chaque capture doit être par valeur ou référence"
    ],
    "S5019": [
        "Capturer explicitement les variables requises",
        "Éviter les captures globales [=] ou [&]",
        "Documenter pourquoi chaque variable est capturée"
    ],
    # Constructeurs
    "S1709": [
        "Ajouter le mot-clé explicit au constructeur",
        "Éviter les conversions implicites non intentionnelles"
    ],
    # Rule of 5
    "S3624": [
        "Implémenter le copy constructor et copy assignment",
        "Implémenter le move constructor et move assignment",
        "Ou déclarer ces méthodes comme = delete"
    ],
    # Code commenté
    "S125": [
        "Supprimer le code commenté",
        "Utiliser le contrôle de version pour l'historique",
        "Créer un ticket si le code doit être réactivé"
    ],
    # Breaks imbriqués
    "S924": [
        "Restructurer la boucle pour éviter les breaks imbriqués",
        "Extraire la logique dans une fonction séparée",
        "Utiliser des flags de contrôle explicites"
    ],
    # Conditions fusionnables
    "S1066": [
        "Fusionner les conditions avec && ou ||",
        "Simplifier la logique conditionnelle",
        "Extraire en méthode si la condition est complexe"
    ],
    # Performance
    "S6045": [
        "Utiliser std::equal_to<> transparent",
        "Permettre les comparaisons hétérogènes",
        "Améliorer la performance des recherches"
    ],
    "S3230": [
        "Utiliser l'initialisation in-class",
        "Simplifier les constructeurs",
        "Centraliser les valeurs par défaut"
    ],
    # Mémoire
    "S5025": [
        "Utiliser std::make_unique pour les allocations",
        "Utiliser std::make_shared si partage nécessaire",
        "Éviter new/delete manuels"
    ],
    # Passage par valeur
    "S1238": [
        "Passer par référence constante (const T&)",
        "Éviter la copie d'objets coûteux",
        "Utiliser std::move si transfert de propriété"
    ],
    # Méthodes spéciales
    "S3490": [
        "Utiliser = default pour les méthodes générées",
        "Laisser le compilateur optimiser",
        "Simplifier le code"
    ],
    # Variables inutilisées
    "S1481": [
        "Supprimer la variable inutilisée",
        "Utiliser [[maybe_unused]] si intentionnel",
        "Vérifier si la variable devait être utilisée"
    ],
    "S1172": [
        "Supprimer le paramètre inutilisé",
        "Renommer en _ si requis par interface",
        "Utiliser [[maybe_unused]] si intentionnel"
    ],
    # Const-correctness
    "S5817": [
        "Ajouter const à la méthode",
        "Garantir que la méthode ne modifie pas l'objet",
        "Améliorer la const-correctness"
    ],
    # Optimisations STL
    "S6003": [
        "Utiliser emplace_back au lieu de push_back",
        "Construire l'objet directement dans le conteneur",
        "Éviter la copie temporaire"
    ],
    "S6009": [
        "Utiliser std::string_view pour les vues de chaînes",
        "Éviter les copies inutiles de strings",
        "Améliorer la performance"
    ],
    # Typedef vs using
    "S5416": [
        "Préférer using à typedef",
        "Utiliser la syntaxe moderne C++11+",
        "Améliorer la lisibilité des alias de types"
    ],
    # Code dupliqué
    "S4144": [
        "Factoriser le code commun dans une fonction",
        "Appliquer le principe DRY",
        "Créer une abstraction si pattern récurrent"
    ],
}

# Mapping du type SonarQube vers la description d'impact
CATEGORY_IMPACT = {
    "Security": "Ce problème peut exposer l'application à des vulnérabilités de sécurité.",
    "Reliability": "Ce problème peut causer des bugs, crashes ou comportements inattendus.",
    "Maintainability": "Ce problème rend le code plus difficile à comprendre, modifier et maintenir.",
}

# Patterns fallback for rules not in RULE_SUGGESTIONS (based on message content)
HOW_SUGGESTIONS_FALLBACK = {
    "cognitive complexity": [
        "Extraire des sous-fonctions pour réduire la complexité",
        "Utiliser des early returns pour éviter l'imbrication",
        "Regrouper les conditions similaires"
    ],
    "nesting": [
        "Inverser les conditions avec early return",
        "Extraire les blocs imbriqués dans des fonctions séparées",
        "Utiliser le pattern Guard Clause"
    ],
    "exception": [
        "Capturer l'exception avec try/catch",
        "Logger l'erreur au lieu de la propager",
        "Utiliser RAII pour la gestion des ressources"
    ],
    "destructor": [
        "Ne jamais lever d'exception dans un destructeur",
        "Capturer et logger les exceptions dans le destructeur",
        "Utiliser noexcept pour marquer le destructeur"
    ],
    "void*": [
        "Utiliser des templates pour la généricité typée",
        "Utiliser std::variant ou std::any selon le cas",
        "Éviter les casts explicites, préférer les types concrets"
    ],
    "empty": [
        "Ajouter un commentaire explicatif si le bloc vide est intentionnel",
        "Supprimer le bloc s'il n'est pas nécessaire",
        "Vérifier si une implémentation est manquante"
    ],
    "parameter": [
        "Regrouper les paramètres liés dans une structure",
        "Utiliser le pattern Builder pour les constructions complexes",
        "Considérer l'utilisation de paramètres par défaut"
    ],
    "hardcoded": [
        "Utiliser des variables d'environnement",
        "Déplacer les valeurs dans un fichier de configuration",
        "Utiliser des constantes nommées pour les valeurs magiques"
    ],
    "credential": [
        "Ne jamais stocker de credentials dans le code",
        "Utiliser un gestionnaire de secrets (Vault, AWS Secrets)",
        "Charger les credentials depuis l'environnement"
    ],
    "buffer": [
        "Utiliser strncpy ou snprintf avec une limite explicite",
        "Vérifier la taille des données avant la copie",
        "Préférer std::string ou std::vector aux tableaux C"
    ],
    "memory": [
        "Utiliser des smart pointers (unique_ptr, shared_ptr)",
        "Libérer la mémoire dans tous les chemins d'exécution",
        "Utiliser RAII pour la gestion automatique"
    ],
    "null": [
        "Vérifier le pointeur avant utilisation",
        "Utiliser std::optional pour les valeurs optionnelles",
        "Documenter les préconditions sur les paramètres"
    ],
    "default": [
        "Consulter la documentation SonarQube pour cette règle",
        "Analyser le contexte spécifique du code",
        "Appliquer les bonnes pratiques recommandées"
    ]
}


def extract_rule_number(rule: str) -> str:
    """Extract rule number from SonarQube rule ID (e.g., 'cpp:S134' -> 'S134')."""
    if ":" in rule:
        return rule.split(":")[-1]
    return rule


def get_rule_language(rule: str) -> str:
    """Extract language from SonarQube rule ID (e.g., 'cpp:S134' -> 'cpp')."""
    if ":" in rule:
        return rule.split(":")[0]
    return "cpp"  # Default to cpp


def build_sonarqube_doc_url(rule: str) -> str:
    """Build the SonarQube documentation URL for a rule.

    Format: https://rules.sonarsource.com/{language}/RSPEC-{number}
    Example: cpp:S134 -> https://rules.sonarsource.com/cpp/RSPEC-134
    """
    language = get_rule_language(rule)
    rule_number = extract_rule_number(rule)

    # Remove 'S' prefix if present to get just the number
    if rule_number.startswith("S"):
        number = rule_number[1:]
    else:
        number = rule_number

    return f"https://rules.sonarsource.com/{language}/RSPEC-{number}"


def get_how_suggestions(rule: str, message: str) -> list[str]:
    """Generate 'how' suggestions based on rule ID or message patterns.

    First tries to match the specific SonarQube rule number,
    then falls back to pattern matching on the message.
    """
    # Try to get suggestions by specific rule number
    rule_number = extract_rule_number(rule)
    if rule_number in RULE_SUGGESTIONS:
        return RULE_SUGGESTIONS[rule_number]

    # Fallback: pattern matching on message content
    text = (rule + " " + message).lower()
    for pattern, suggestions in HOW_SUGGESTIONS_FALLBACK.items():
        if pattern in text:
            return suggestions

    return HOW_SUGGESTIONS_FALLBACK["default"]


def generate_mermaid_diagram(category: str, message: str) -> str:
    """Generate a Mermaid diagram based on the issue category.

    Returns an appropriate diagram type:
    - Security: sequenceDiagram showing attack flow
    - Reliability: graph TD showing error flow
    - Maintainability: mindmap showing impacts
    """
    if category == "Security":
        return """```mermaid
sequenceDiagram
    participant Attaquant
    participant Application
    participant Système

    Attaquant->>Application: Données malveillantes
    Application->>Système: Traitement non sécurisé
    Note over Système: Vulnérabilité exploitée
    Système-->>Attaquant: Accès non autorisé
```"""
    elif category == "Reliability":
        return """```mermaid
graph TD
    A[Entrée problématique] --> B[Code vulnérable]
    B --> C{Condition d'erreur}
    C -->|Oui| D[❌ Comportement indéfini]
    C -->|Non| E[✅ OK]
    style D fill:#f66,stroke:#333
```"""
    else:  # Maintainability
        # Extract key words from message for mindmap
        short_msg = message[:30] if len(message) > 30 else message
        return f"""```mermaid
mindmap
  root(({short_msg}))
    Tests difficiles
      Couverture insuffisante
      Cas limites non testés
    Maintenance coûteuse
      Temps de compréhension élevé
      Risque de régression
    Dette technique
      Code difficile à refactorer
      Documentation manquante
```"""


def get_file_extension(file_path: str) -> str:
    """Get the file extension for syntax highlighting."""
    if file_path.endswith((".cpp", ".cc", ".cxx", ".hpp", ".h")):
        return "cpp"
    elif file_path.endswith((".c", ".h")):
        return "c"
    elif file_path.endswith((".py",)):
        return "python"
    elif file_path.endswith((".js", ".jsx")):
        return "javascript"
    elif file_path.endswith((".ts", ".tsx")):
        return "typescript"
    elif file_path.endswith((".java",)):
        return "java"
    elif file_path.endswith((".go",)):
        return "go"
    elif file_path.endswith((".rs",)):
        return "rust"
    else:
        return "cpp"  # Default


def issue_to_web_format(issue: "Issue", index: int) -> dict:
    """Transform a SonarQube issue to web JSON format with where/why/how.

    Generates detailed markdown content for each field:
    - where: Location of the issue with file path, line number, and code placeholder
    - why: Explanation of the problem with Mermaid diagram, rule reference and impact
    - how: Suggested solutions based on the specific rule with steps
    """
    # Generate unique ID
    issue_id = f"SONAR-{index:03d}"

    # Determine isBug based on type and severity
    # Only true if it causes crash/freeze (Reliability bugs or critical security issues)
    is_bug = issue.category == "Reliability" and issue.severity in ("Blocker", "Critical")

    # Build documentation URL
    doc_url = build_sonarqube_doc_url(issue.rule)

    # Get category-specific impact description
    impact_description = CATEGORY_IMPACT.get(
        issue.category,
        "Ce problème affecte la qualité du code."
    )

    # Get file extension for code blocks
    lang = get_file_extension(issue.file)

    # =========================================================================
    # BUILD 'where' MARKDOWN - WITH CODE PLACEHOLDER
    # =========================================================================
    line_str = str(issue.line) if issue.line else "Non spécifiée"
    where_md = f"""## Localisation

Le problème se trouve dans `{issue.file}` à la ligne {line_str}.

```{lang}
// Code à la ligne {line_str}
// Le snippet de code réel sera extrait par l'agent SONAR lors de l'enrichissement
// Message SonarQube : {issue.message[:80]}
```

### Contexte

Cette issue a été détectée par l'analyse statique SonarQube sur la règle **{issue.rule}**.

**Fichier** : `{issue.file}`
**Ligne** : {line_str}
**Sévérité** : {issue.severity}

> **Note** : L'agent SONAR enrichira ce champ avec le code source réel et le contexte AgentDB."""

    # =========================================================================
    # BUILD 'why' MARKDOWN - WITH MERMAID DIAGRAM
    # =========================================================================
    mermaid_diagram = generate_mermaid_diagram(issue.category, issue.message)

    why_md = f"""## Pourquoi c'est un problème

{issue.message}

**Règle SonarQube** : [{issue.rule}]({doc_url})
**Catégorie** : {issue.category}
**Sévérité** : {issue.severity}
**Effort estimé** : {issue.effort}

### Visualisation du problème

{mermaid_diagram}

### Impact

{impact_description}

### Risques

| Risque | Probabilité | Impact |
|--------|-------------|--------|
| Dette technique accrue | Haute | Moyen |
| Bugs lors de modifications | Moyenne | Majeur |
| Temps de maintenance élevé | Haute | Moyen |"""

    # =========================================================================
    # BUILD 'how' MARKDOWN - WITH STEPS
    # =========================================================================
    suggestions = get_how_suggestions(issue.rule, issue.message)
    how_lines = [f"{i+1}. {s}" for i, s in enumerate(suggestions)]

    how_md = f"""## Comment corriger

### Solution suggérée

{chr(10).join(how_lines)}

### Processus de correction

```mermaid
graph LR
    A[Identifier le problème] --> B[Appliquer la solution]
    B --> C[Tester le changement]
    C --> D[Valider avec SonarQube]
    style D fill:#6f6,stroke:#333
```

### Étapes détaillées

1. **Localiser** le code problématique dans `{issue.file}:{issue.line}`
2. **Comprendre** la règle [{issue.rule}]({doc_url})
3. **Appliquer** la correction selon les suggestions ci-dessus
4. **Tester** que le comportement est préservé
5. **Vérifier** avec SonarQube que l'issue est résolue

### Ressources

- [Documentation SonarQube {issue.rule}]({doc_url})

### Temps estimé

**{issue.effort}** pour corriger cette issue."""

    return {
        "id": issue_id,
        "source": ["sonarqube"],
        "title": issue.message[:100] + ("..." if len(issue.message) > 100 else ""),
        "severity": issue.severity,
        "category": issue.category,
        "status": "pending",
        "isBug": is_bug,
        "file": issue.file,
        "line": issue.line,
        "where": where_md,
        "why": why_md,
        "how": how_md,
        "effort": issue.effort,
        "rule": issue.rule
    }


def generate_json_issues(report: "SonarReport") -> list[dict]:
    """Generate JSON list of issues in web format."""
    issues_json = []

    for idx, issue in enumerate(report.issues, start=1):
        issues_json.append(issue_to_web_format(issue, idx))

    return issues_json


# =============================================================================
# AGENTDB INTEGRATION
# =============================================================================

def store_in_agentdb(db_path: Path, report: SonarReport, commit: str | None, branch: str | None):
    """Store scan metrics in AgentDB for historical tracking."""

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Create table if not exists
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS sonar_scans (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            commit_hash TEXT,
            branch TEXT,
            blockers INTEGER DEFAULT 0,
            criticals INTEGER DEFAULT 0,
            majors INTEGER DEFAULT 0,
            minors INTEGER DEFAULT 0,
            infos INTEGER DEFAULT 0,
            bugs INTEGER DEFAULT 0,
            vulnerabilities INTEGER DEFAULT 0,
            code_smells INTEGER DEFAULT 0,
            total_issues INTEGER DEFAULT 0,
            scanned_at TEXT DEFAULT (datetime('now'))
        )
    """)

    # Insert scan data
    cursor.execute("""
        INSERT INTO sonar_scans (
            commit_hash, branch,
            blockers, criticals, majors, minors, infos,
            bugs, vulnerabilities, code_smells,
            total_issues, scanned_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'))
    """, (
        commit,
        branch,
        report.severity_count("Blocker"),
        report.severity_count("Critical"),
        report.severity_count("Major"),
        report.severity_count("Minor"),
        report.severity_count("Info"),
        report.category_count("Reliability"),      # BUG → Reliability
        report.category_count("Security"),         # VULNERABILITY → Security
        report.category_count("Maintainability"),  # CODE_SMELL → Maintainability
        report.total,
    ))

    conn.commit()
    conn.close()

    print(f"[AgentDB] Scan enregistré dans {db_path}", file=sys.stderr)


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Transform SonarQube JSON report to Markdown summary",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    # Full project audit (all issues)
    python transform-sonar.py issues.json

    # PR/Commit review (filter to diff files only)
    python transform-sonar.py issues.json --files "src/A.cpp,src/B.hpp,src/C.cpp"

    # With metadata
    python transform-sonar.py issues.json --files "src/A.cpp" --commit abc123 --branch feature/foo

    # Custom output path
    python transform-sonar.py issues.json --output .claude/reports/sonar-summary.md

    # Store metrics in AgentDB
    python transform-sonar.py issues.json --db .claude/agentdb/db.sqlite
        """
    )

    parser.add_argument(
        "input",
        type=str,
        help="Path to SonarQube JSON file"
    )
    parser.add_argument(
        "--output", "-o",
        type=str,
        default=None,
        help="Output Markdown file path (default: same name with .md extension)"
    )
    parser.add_argument(
        "--files", "-f",
        type=str,
        default=None,
        help="Comma-separated list of files to filter (for PR/diff mode). "
             "Only issues in these files will be included."
    )
    parser.add_argument(
        "--commit", "-c",
        type=str,
        default=None,
        help="Commit hash for metadata"
    )
    parser.add_argument(
        "--branch", "-b",
        type=str,
        default=None,
        help="Branch name for metadata"
    )
    parser.add_argument(
        "--top", "-t",
        type=int,
        default=20,
        help="Number of files/rules in top lists (default: 20)"
    )
    parser.add_argument(
        "--db",
        type=str,
        default=None,
        help="AgentDB SQLite path to store metrics"
    )
    parser.add_argument(
        "--since", "-s",
        type=str,
        default=None,
        help="Filter issues by creation date. Supports: relative durations (24h, 48h, 7d, 2w), "
             "ISO dates (2025-12-10, 2025-12-10T14:30:00+01:00), or 'none'/'all' to disable. "
             "Default: 48h"
    )

    args = parser.parse_args()

    # Parse file filter
    file_filter: set[str] | None = None
    if args.files:
        file_filter = {f.strip() for f in args.files.split(",") if f.strip()}

    # Parse since date filter
    since_date = parse_since_argument(args.since)
    if since_date:
        print(f"Filtrage temporel: issues depuis {since_date.strftime('%Y-%m-%d %H:%M')} UTC", file=sys.stderr)
    else:
        print("Filtrage temporel: désactivé (toutes les issues)", file=sys.stderr)

    # Validate input file
    input_path = Path(args.input)
    if not input_path.exists():
        print(f"Erreur: Fichier introuvable: {input_path}", file=sys.stderr)
        sys.exit(1)

    # Determine output path
    if args.output:
        output_path = Path(args.output)
    else:
        output_path = input_path.with_suffix(".md")

    # Parse JSON
    try:
        report, total_before_filter, filtered_by_date = parse_sonar_json(
            input_path, file_filter, since_date
        )
    except json.JSONDecodeError as e:
        print(f"Erreur: JSON mal formé: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Erreur lors du parsing: {e}", file=sys.stderr)
        sys.exit(1)

    # Handle empty report
    if report.total == 0:
        if filtered_by_date > 0:
            print(f"Info: {filtered_by_date} issues filtrées (créées avant la date limite).", file=sys.stderr)
        if file_filter:
            print(f"Info: Aucune issue SonarQube sur les {len(file_filter)} fichiers analysés.", file=sys.stderr)
        else:
            print("Info: Aucune issue active détectée dans le rapport.", file=sys.stderr)

    # Generate Markdown
    markdown = generate_markdown(
        report=report,
        commit=args.commit,
        branch=args.branch,
        input_file=str(input_path),
        top_n=args.top,
        file_filter=file_filter,
        total_before_filter=total_before_filter
    )

    # Write markdown output
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(markdown)

    print(f"Rapport généré: {output_path}", file=sys.stderr)
    print(f"  - Total issues: {report.total}", file=sys.stderr)
    if filtered_by_date > 0:
        print(f"  - Filtrées (anciennes): {filtered_by_date}", file=sys.stderr)
    print(f"  - Blockers: {report.severity_count('Blocker')}", file=sys.stderr)
    print(f"  - Criticals: {report.severity_count('Critical')}", file=sys.stderr)
    print(f"  - Majors: {report.severity_count('Major')}", file=sys.stderr)

    # Write JSON output for web-synthesizer
    json_output_path = output_path.with_name("sonar-issues.json")
    json_issues = generate_json_issues(report)
    with open(json_output_path, "w", encoding="utf-8") as f:
        json.dump(json_issues, f, indent=2, ensure_ascii=False)

    print(f"Rapport JSON généré: {json_output_path}", file=sys.stderr)
    print(f"  - Issues JSON: {len(json_issues)}", file=sys.stderr)

    # Store in AgentDB if requested
    if args.db:
        db_path = Path(args.db)
        if not db_path.parent.exists():
            print(f"Erreur: Dossier AgentDB introuvable: {db_path.parent}", file=sys.stderr)
            sys.exit(1)
        store_in_agentdb(db_path, report, args.commit, args.branch)

    # Exit with appropriate code
    if report.severity_count("Blocker") > 0:
        sys.exit(2)  # Blockers present
    elif report.severity_count("Critical") > 0:
        sys.exit(1)  # Criticals present
    else:
        sys.exit(0)  # OK


if __name__ == "__main__":
    main()
