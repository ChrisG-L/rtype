"""
AgentDB - Indexeur de code source.

Ce module est responsable de :
- Parser les fichiers source pour extraire les symboles (via ctags)
- Détecter les relations entre symboles (appels, includes, etc.)
- Calculer les métriques de code (complexité, lignes, etc.)
- Analyser l'historique Git (activité, contributeurs)
- Détecter les fichiers critiques

Parsers supportés :
- C/C++ : Universal Ctags (obligatoire pour C/C++)
- Python : module ast (natif)

Usage:
    from agentdb.db import DatabaseManager
    from agentdb.indexer import CodeIndexer, IndexerConfig

    config = IndexerConfig(project_root=Path("."))

    with DatabaseManager(".claude/agentdb/db.sqlite") as db:
        indexer = CodeIndexer(db, config)

        # Indexer un seul fichier
        result = indexer.index_file("src/main.c")
        print(f"Indexed {result.symbols_count} symbols")

        # Indexer un répertoire
        results = indexer.index_directory("src/", recursive=True)

        # Réindexer des fichiers modifiés
        results = indexer.reindex_files(["src/modified.c", "src/new.c"])
"""

from __future__ import annotations

import fnmatch
import hashlib
import json
import logging
import re
import shutil
import subprocess
import time
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any, Optional

from .db import Database, DatabaseManager
from .models import (
    File,
    Symbol,
    Relation,
    FileRelation,
    SymbolKind,
    RelationType,
)
from .crud import (
    FileRepository,
    SymbolRepository,
    RelationRepository,
    FileRelationRepository,
)

# Configuration du logging
logger = logging.getLogger("agentdb.indexer")


# =============================================================================
# DATA CLASSES
# =============================================================================

@dataclass
class IndexResult:
    """
    Résultat de l'indexation d'un fichier.

    Attributes:
        file_path: Chemin du fichier indexé
        file_id: ID du fichier dans la base (None si erreur)
        symbols_count: Nombre de symboles extraits
        relations_count: Nombre de relations extraites
        duration_ms: Temps d'indexation en millisecondes
        errors: Liste des erreurs rencontrées
        warnings: Liste des avertissements
    """
    file_path: str
    file_id: Optional[int] = None
    symbols_count: int = 0
    relations_count: int = 0
    duration_ms: float = 0.0
    errors: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)

    @property
    def success(self) -> bool:
        """True si l'indexation a réussi (pas d'erreurs)."""
        return len(self.errors) == 0

    def to_dict(self) -> dict[str, Any]:
        """Convertit en dictionnaire."""
        return {
            "file_path": self.file_path,
            "file_id": self.file_id,
            "symbols_count": self.symbols_count,
            "relations_count": self.relations_count,
            "duration_ms": self.duration_ms,
            "errors": self.errors,
            "warnings": self.warnings,
            "success": self.success,
        }


@dataclass
class IndexerConfig:
    """
    Configuration de l'indexeur.

    Attributes:
        project_root: Racine du projet
        extensions: Extensions à indexer par langage
        exclude_patterns: Patterns glob à exclure
        critical_paths: Patterns des chemins critiques
        high_importance_paths: Patterns haute importance
        ctags_path: Chemin vers l'exécutable ctags (auto-détecté si None)
    """
    project_root: Path = field(default_factory=lambda: Path("."))
    extensions: dict[str, list[str]] = field(default_factory=dict)
    exclude_patterns: list[str] = field(default_factory=list)
    critical_paths: list[str] = field(default_factory=list)
    high_importance_paths: list[str] = field(default_factory=list)
    ctags_path: Optional[str] = None

    def __post_init__(self):
        # Valeurs par défaut si non fournies
        if not self.extensions:
            self.extensions = {
                "c": [".c", ".h"],
                "cpp": [".cpp", ".hpp", ".cc", ".hh", ".cxx", ".hxx"],
                "python": [".py"],
                "javascript": [".js", ".jsx", ".ts", ".tsx"],
            }
        if not self.exclude_patterns:
            self.exclude_patterns = [
                "build/**", "dist/**", "vendor/**", "node_modules/**",
                "**/*.min.js", "**/*.generated.*", ".git/**",
                ".claude/agentdb/**", "__pycache__/**", "*.pyc",
                ".venv/**", "venv/**", ".env/**", "env/**",
            ]
        if not self.critical_paths:
            self.critical_paths = [
                "**/security/**", "**/auth/**", "**/crypto/**",
                "**/*password*", "**/*secret*", "**/*token*",
            ]
        if not self.high_importance_paths:
            self.high_importance_paths = [
                "**/core/**", "**/api/**", "**/main.*",
            ]

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> "IndexerConfig":
        """Crée une config depuis un dictionnaire."""
        return cls(
            project_root=Path(data.get("project_root", ".")),
            extensions=data.get("extensions", {}),
            exclude_patterns=data.get("exclude_patterns", []),
            critical_paths=data.get("critical_paths", []),
            high_importance_paths=data.get("high_importance_paths", []),
            ctags_path=data.get("ctags_path"),
        )


# =============================================================================
# CTAGS FUNCTIONS
# =============================================================================

def check_ctags_available(ctags_path: Optional[str] = None) -> tuple[bool, str]:
    """
    Vérifie si ctags est disponible.

    Args:
        ctags_path: Chemin optionnel vers ctags

    Returns:
        (disponible, chemin_ou_message_erreur)
    """
    # Chercher ctags
    if ctags_path:
        path = ctags_path
    else:
        # Chercher dans PATH
        path = shutil.which("ctags") or shutil.which("universal-ctags")

    if not path:
        return False, "ctags not found in PATH. Install with: sudo apt install universal-ctags"

    # Vérifier que c'est Universal Ctags (pas le vieux exuberant-ctags)
    try:
        result = subprocess.run(
            [path, "--version"],
            capture_output=True,
            text=True,
            timeout=5
        )
        version_output = result.stdout.lower()
        if "universal ctags" in version_output or "universal-ctags" in version_output:
            return True, path
        elif "exuberant" in version_output:
            return False, f"{path} is Exuberant Ctags (old). Please install Universal Ctags."
        else:
            # Probablement OK
            return True, path
    except Exception as e:
        return False, f"Error checking ctags: {e}"


def run_ctags(file_path: str, ctags_path: str = "ctags", language: str = None) -> list[dict[str, Any]]:
    """
    Exécute ctags sur un fichier et retourne les tags au format JSON.

    Args:
        file_path: Chemin du fichier à analyser
        ctags_path: Chemin vers l'exécutable ctags
        language: Langage du fichier (c, cpp, etc.)

    Returns:
        Liste des tags extraits (dict par tag)

    Raises:
        FileNotFoundError: Si le fichier n'existe pas
        RuntimeError: Si ctags échoue

    Example:
        >>> tags = run_ctags("src/main.c", language="c")
        >>> for tag in tags:
        ...     print(f"{tag['name']} ({tag['kind']}) at line {tag['line']}")
    """
    path = Path(file_path)
    if not path.exists():
        raise FileNotFoundError(f"File not found: {file_path}")

    # Options ctags optimisées pour extraction complète
    # --fields=+iaS : i=inheritance, a=access, S=signature
    # --extras=+q : qualified tags (namespace::class::method)
    # --c++-kinds=+p : inclure prototypes pour C++
    cmd = [
        ctags_path,
        "--output-format=json",
        "--fields=+iaSneSKlZ",  # i=inheritance, a=access, S=signature, n=line, e=end, K=kind, l=lang, Z=scope
        "--extras=+q",  # qualified names
        "--kinds-all=*",
    ]

    # Options spécifiques au langage
    if language in ("c", "cpp"):
        cmd.extend([
            "--c-kinds=+p+x+l",  # +p=prototypes, +x=externs, +l=local vars
            "--c++-kinds=+p+l",  # +p=prototypes, +l=local vars
        ])

    cmd.extend([
        "-o", "-",  # Sortie sur stdout
        str(path.absolute())
    ])

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30
        )

        # ctags peut retourner 0 même avec des warnings, on vérifie stdout
        if result.returncode != 0 and not result.stdout.strip():
            error_msg = result.stderr.strip() or "Unknown ctags error"
            raise RuntimeError(f"ctags failed: {error_msg}")

        return parse_ctags_output(result.stdout)

    except subprocess.TimeoutExpired:
        raise RuntimeError(f"ctags timed out processing {file_path}")
    except FileNotFoundError:
        raise RuntimeError(f"ctags not found at {ctags_path}")


def parse_ctags_output(output: str) -> list[dict[str, Any]]:
    """
    Parse la sortie JSON de ctags (une ligne JSON par tag).

    Args:
        output: Sortie brute de ctags --output-format=json

    Returns:
        Liste de dictionnaires, un par tag

    Example:
        >>> output = '{"name":"main","kind":"function","line":10}\\n'
        >>> tags = parse_ctags_output(output)
        >>> tags[0]["name"]
        'main'
    """
    tags = []
    for line in output.strip().split("\n"):
        line = line.strip()
        if not line:
            continue
        try:
            tag = json.loads(line)
            tags.append(tag)
        except json.JSONDecodeError as e:
            logger.warning(f"Failed to parse ctags line: {line[:100]}... - {e}")
    return tags


def ctags_to_symbols(tags: list[dict[str, Any]], file_id: int, file_content: str = None) -> list[Symbol]:
    """
    Convertit les tags ctags en objets Symbol avec tous les champs remplis.

    Args:
        tags: Liste des tags de ctags
        file_id: ID du fichier dans la base
        file_content: Contenu du fichier (pour calcul de complexité)

    Returns:
        Liste de Symbol
    """
    symbols = []

    # Mapping des kinds ctags vers SymbolKind
    kind_mapping = {
        "function": "function",
        "method": "method",
        "class": "class",
        "struct": "struct",
        "union": "union",
        "enum": "enum",
        "enumerator": "constant",
        "typedef": "typedef",
        "macro": "macro",
        "variable": "variable",
        "externvar": "variable",
        "member": "field",
        "field": "field",
        "prototype": "function",
        "namespace": "namespace",
        "interface": "interface",
        "property": "property",
        "constant": "constant",
        "parameter": "parameter",
        "local": "variable",
    }

    # Pré-calculer les lignes pour la complexité
    lines = file_content.split("\n") if file_content else []

    for tag in tags:
        name = tag.get("name", "")
        if not name:
            continue

        kind_raw = tag.get("kind", "").lower()
        kind = kind_mapping.get(kind_raw, "variable")

        # Extraire la signature pour les fonctions
        signature = tag.get("signature", "")
        pattern = tag.get("pattern", "")

        # Si pas de signature, essayer de l'extraire du pattern
        if not signature and kind in ("function", "method", "prototype"):
            # Le pattern est souvent /^type name(params)$/
            if pattern:
                # Nettoyer le pattern (enlever /^ et $/)
                clean_pattern = pattern.strip("/^$")
                sig_match = re.search(r'(\w[\w\s\*]*\s+\*?\s*' + re.escape(name) + r'\s*\([^)]*\))', clean_pattern)
                if sig_match:
                    signature = sig_match.group(1).strip()

        # Extraire le type de retour
        return_type = tag.get("typeref", "")
        if not return_type and kind in ("function", "method") and signature:
            # Essayer d'extraire le type de retour de la signature
            match = re.match(r'^([\w\s\*]+?)\s+\*?\s*' + re.escape(name), signature)
            if match:
                return_type = match.group(1).strip()

        # Déterminer la visibilité depuis le champ 'access' de ctags
        access = tag.get("access", "").lower()
        visibility = "public"
        if access in ("private", "protected", "public"):
            visibility = access
        elif "static" in pattern.lower():
            visibility = "private"  # static en C = portée fichier = private
        elif tag.get("file"):  # tag marqué comme file-scope
            visibility = "private"

        # Extraire le scope (namespace, class, struct)
        scope = tag.get("scope", "")
        scope_kind = tag.get("scopeKind", "")

        # Construire le qualified_name
        qualified_name = None
        if scope:
            qualified_name = f"{scope}::{name}"
        elif tag.get("extras") and "qualified" in str(tag.get("extras", "")):
            qualified_name = name  # Déjà qualifié

        # Calculer la complexité pour les fonctions
        complexity = 0
        line_start = tag.get("line")
        line_end = tag.get("end")

        if kind in ("function", "method") and lines and line_start:
            complexity = _calculate_function_complexity(lines, line_start, line_end)

        # Extraire les informations d'héritage
        inherits = tag.get("inherits", "")
        base_classes = None
        if inherits:
            base_classes = [b.strip() for b in inherits.split(",")]

        # Détecter si c'est static/inline/extern
        is_static = "static" in pattern.lower() if pattern else False
        is_inline = "inline" in pattern.lower() if pattern else False
        is_extern = tag.get("kind") == "externvar" or ("extern" in pattern.lower() if pattern else False)

        symbol = Symbol(
            file_id=file_id,
            name=name,
            qualified_name=qualified_name,
            kind=kind,
            line_start=line_start,
            line_end=line_end,
            signature=signature if signature else None,
            return_type=return_type if return_type else None,
            visibility=visibility,
            is_static=is_static,
            is_inline=is_inline,
            is_exported=not is_static and visibility == "public",
            complexity=complexity,
            base_classes_json=json.dumps(base_classes) if base_classes else None,
        )
        symbols.append(symbol)

    return symbols


def _calculate_function_complexity(lines: list[str], start: int, end: int = None) -> int:
    """
    Calcule la complexité cyclomatique d'une fonction.

    Args:
        lines: Lignes du fichier
        start: Ligne de début (1-indexed)
        end: Ligne de fin (1-indexed), ou None pour estimer

    Returns:
        Complexité cyclomatique
    """
    if not lines or start is None:
        return 0

    # Ajuster pour index 0-based
    start_idx = max(0, start - 1)
    end_idx = min(len(lines), (end or start + 100))

    # Extraire le code de la fonction
    func_lines = lines[start_idx:end_idx]
    func_code = "\n".join(func_lines)

    # Compter les points de décision
    complexity = 1  # Base

    patterns = [
        r'\bif\s*\(',
        r'\belse\s+if\s*\(',
        r'\bfor\s*\(',
        r'\bwhile\s*\(',
        r'\bdo\s*\{',
        r'\bcase\s+\S+\s*:',
        r'\bcatch\s*\(',
        r'\b\?\s*[^:]+\s*:',  # ternaire
        r'\s&&\s',
        r'\s\|\|\s',
    ]

    for pattern in patterns:
        matches = re.findall(pattern, func_code)
        complexity += len(matches)

    return complexity


# =============================================================================
# LINE COUNTING
# =============================================================================

def count_lines(file_path: str, language: Optional[str] = None) -> dict[str, int]:
    """
    Compte les lignes d'un fichier : total, code, commentaires, blanches.

    Args:
        file_path: Chemin du fichier
        language: Langage (pour déterminer les commentaires)

    Returns:
        Dict avec total, code, comment, blank

    Example:
        >>> counts = count_lines("src/main.c", "c")
        >>> print(f"Code: {counts['code']}, Comments: {counts['comment']}")
    """
    try:
        content = Path(file_path).read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        logger.warning(f"Cannot read {file_path}: {e}")
        return {"total": 0, "code": 0, "comment": 0, "blank": 0}

    lines = content.split("\n")
    total = len(lines)
    blank = 0
    comment = 0
    code = 0

    # Déterminer les patterns de commentaires selon le langage
    if language in ("c", "cpp", "javascript"):
        single_comment = "//"
        multi_start = "/*"
        multi_end = "*/"
    elif language == "python":
        single_comment = "#"
        multi_start = '"""'
        multi_end = '"""'
    else:
        # Défaut: style C
        single_comment = "//"
        multi_start = "/*"
        multi_end = "*/"

    in_multiline = False

    for line in lines:
        stripped = line.strip()

        if not stripped:
            blank += 1
            continue

        # Gestion des commentaires multi-lignes
        if in_multiline:
            comment += 1
            if multi_end in stripped:
                in_multiline = False
            continue

        if multi_start in stripped:
            comment += 1
            if multi_end not in stripped[stripped.index(multi_start) + len(multi_start):]:
                in_multiline = True
            continue

        # Commentaire simple
        if stripped.startswith(single_comment):
            comment += 1
            continue

        # Sinon c'est du code
        code += 1

    return {
        "total": total,
        "code": code,
        "comment": comment,
        "blank": blank,
    }


# =============================================================================
# COMPLEXITY CALCULATION
# =============================================================================

def calculate_complexity(file_path: str, language: Optional[str] = None) -> dict[str, Any]:
    """
    Calcule la complexité cyclomatique d'un fichier.

    La complexité cyclomatique compte les points de décision :
    - if, else if, elif
    - for, while, do-while
    - switch case
    - && ||
    - ternaire ? :
    - catch/except

    Args:
        file_path: Chemin du fichier
        language: Langage pour adapter l'analyse

    Returns:
        Dict avec sum (total), avg (moyenne par fonction), max (complexité max)

    Example:
        >>> cx = calculate_complexity("src/main.c", "c")
        >>> print(f"Max complexity: {cx['max']}")
    """
    try:
        content = Path(file_path).read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        logger.warning(f"Cannot read {file_path}: {e}")
        return {"sum": 0, "avg": 0.0, "max": 0}

    # Patterns de complexité pour C/C++/JS
    if language in ("c", "cpp", "javascript"):
        patterns = [
            r'\bif\s*\(',
            r'\belse\s+if\s*\(',
            r'\bfor\s*\(',
            r'\bwhile\s*\(',
            r'\bdo\s*\{',
            r'\bcase\s+',
            r'\bcatch\s*\(',
            r'\?\s*[^:]+\s*:',  # ternaire
            r'&&',
            r'\|\|',
        ]
    elif language == "python":
        patterns = [
            r'\bif\s+',
            r'\belif\s+',
            r'\bfor\s+',
            r'\bwhile\s+',
            r'\bexcept\s*:',
            r'\bexcept\s+\w',
            r'\band\b',
            r'\bor\b',
            r'\bif\s+\S+\s+else\s+',  # ternaire Python
        ]
    else:
        # Patterns génériques
        patterns = [
            r'\bif\b', r'\bfor\b', r'\bwhile\b', r'\bcase\b',
            r'&&', r'\|\|',
        ]

    total_complexity = 1  # Base complexity

    for pattern in patterns:
        matches = re.findall(pattern, content)
        total_complexity += len(matches)

    # Estimer le nombre de fonctions pour la moyenne
    if language in ("c", "cpp"):
        func_pattern = r'\b\w+\s+\*?\s*\w+\s*\([^)]*\)\s*\{'
    elif language == "python":
        func_pattern = r'\bdef\s+\w+'
    else:
        func_pattern = r'\bfunction\b|\bdef\b|\bfunc\b'

    functions = re.findall(func_pattern, content)
    func_count = max(len(functions), 1)

    return {
        "sum": total_complexity,
        "avg": round(total_complexity / func_count, 2),
        "max": total_complexity,  # Approximation - idéalement par fonction
    }


# =============================================================================
# RELATION EXTRACTION
# =============================================================================

def extract_includes(file_path: str, language: Optional[str] = None) -> list[dict[str, Any]]:
    """
    Extrait les directives #include ou import d'un fichier.

    Args:
        file_path: Chemin du fichier
        language: Langage du fichier

    Returns:
        Liste de dict avec: included_file, line_number, is_system

    Example:
        >>> includes = extract_includes("src/main.c", "c")
        >>> for inc in includes:
        ...     print(f"Line {inc['line']}: {inc['path']}")
    """
    try:
        content = Path(file_path).read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        logger.warning(f"Cannot read {file_path}: {e}")
        return []

    includes = []
    lines = content.split("\n")

    if language in ("c", "cpp"):
        # Pattern pour #include
        include_pattern = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]')

        for i, line in enumerate(lines, 1):
            match = include_pattern.match(line)
            if match:
                bracket = match.group(1)
                path = match.group(2)
                includes.append({
                    "path": path,
                    "line": i,
                    "is_system": bracket == "<",
                })

    elif language == "python":
        # Pattern pour import et from ... import
        import_pattern = re.compile(r'^\s*import\s+([\w.]+)')
        from_pattern = re.compile(r'^\s*from\s+([\w.]+)\s+import')

        for i, line in enumerate(lines, 1):
            match = import_pattern.match(line)
            if match:
                includes.append({
                    "path": match.group(1),
                    "line": i,
                    "is_system": not match.group(1).startswith("."),
                })
                continue

            match = from_pattern.match(line)
            if match:
                includes.append({
                    "path": match.group(1),
                    "line": i,
                    "is_system": not match.group(1).startswith("."),
                })

    elif language == "javascript":
        # Pattern pour import et require
        import_pattern = re.compile(r'''^\s*import\s+.*?from\s+['"]([\w./@-]+)['"]''')
        require_pattern = re.compile(r'''require\s*\(\s*['"]([\w./@-]+)['"]\s*\)''')

        for i, line in enumerate(lines, 1):
            match = import_pattern.match(line)
            if match:
                path = match.group(1)
                includes.append({
                    "path": path,
                    "line": i,
                    "is_system": not path.startswith("."),
                })
                continue

            match = require_pattern.search(line)
            if match:
                path = match.group(1)
                includes.append({
                    "path": path,
                    "line": i,
                    "is_system": not path.startswith("."),
                })

    return includes


def extract_python_calls(
    file_path: str,
    symbols: list,
    all_symbols: dict[str, int]
) -> list[dict[str, Any]]:
    """
    Extrait les appels de fonction depuis un fichier Python en utilisant l'AST.

    Cette méthode est précise car elle utilise l'AST Python natif.

    Args:
        file_path: Chemin du fichier Python
        symbols: Symboles définis dans ce fichier (avec line_start, line_end)
        all_symbols: Dict {symbol_name: symbol_id} de tous les symboles connus

    Returns:
        Liste de dict avec: caller, callee, line
    """
    import ast

    try:
        content = Path(file_path).read_text(encoding="utf-8")
        tree = ast.parse(content, filename=file_path)
    except Exception as e:
        logger.warning(f"Cannot parse {file_path} for call extraction: {e}")
        return []

    calls = []

    # Construire un index des fonctions/méthodes locales avec leurs plages de lignes
    local_functions = {}
    for sym in symbols:
        if hasattr(sym, 'kind'):
            kind = sym.kind
            name = sym.name
            line_start = sym.line_start
            line_end = sym.line_end
        else:
            kind = sym.get('kind', '')
            name = sym.get('name', '')
            line_start = sym.get('line_start')
            line_end = sym.get('line_end')

        if kind in ("function", "method") and line_start:
            local_functions[name] = {
                "start": line_start,
                "end": line_end or line_start + 500,
            }

    class CallVisitor(ast.NodeVisitor):
        """Visiteur AST pour extraire les appels de fonction."""

        def __init__(self):
            self.current_function = None
            self.current_class = None

        def visit_FunctionDef(self, node):
            old_func = self.current_function
            self.current_function = node.name
            self.generic_visit(node)
            self.current_function = old_func

        def visit_AsyncFunctionDef(self, node):
            self.visit_FunctionDef(node)

        def visit_ClassDef(self, node):
            old_class = self.current_class
            self.current_class = node.name
            self.generic_visit(node)
            self.current_class = old_class

        def visit_Call(self, node):
            callee_name = None

            # Appel simple: func()
            if isinstance(node.func, ast.Name):
                callee_name = node.func.id

            # Appel de méthode: obj.method() - on prend juste le nom de la méthode
            elif isinstance(node.func, ast.Attribute):
                callee_name = node.func.attr

            if callee_name and self.current_function:
                # Vérifier que le callee est un symbole connu
                if callee_name in all_symbols or callee_name in local_functions:
                    # Éviter les auto-appels
                    if callee_name != self.current_function:
                        calls.append({
                            "caller": self.current_function,
                            "callee": callee_name,
                            "line": node.lineno,
                        })

            self.generic_visit(node)

    visitor = CallVisitor()
    visitor.visit(tree)

    return calls


def extract_calls(
    file_path: str,
    symbols: list,
    all_symbols: dict[str, int],
    language: str = None
) -> list[dict[str, Any]]:
    """
    Extrait les appels de fonction depuis un fichier source.

    Utilise l'AST pour Python, regex pour C/C++/JS.

    Args:
        file_path: Chemin du fichier source
        symbols: Symboles définis dans ce fichier
        all_symbols: Dict {symbol_name: symbol_id} de tous les symboles connus
        language: Langage du fichier (python, c, cpp, javascript)

    Returns:
        Liste de dict avec: caller, callee, line

    Example:
        >>> calls = extract_calls("src/main.py", local_symbols, global_symbols, "python")
        >>> for call in calls:
        ...     print(f"{call['caller']} calls {call['callee']} at line {call['line']}")
    """
    # Détection du langage si non fourni
    if language is None:
        ext = Path(file_path).suffix.lower()
        if ext in (".py", ".pyi"):
            language = "python"
        elif ext in (".c", ".h"):
            language = "c"
        elif ext in (".cpp", ".hpp", ".cc", ".hh", ".cxx", ".hxx"):
            language = "cpp"
        elif ext in (".js", ".jsx", ".ts", ".tsx"):
            language = "javascript"

    # Pour Python, utiliser l'AST
    if language == "python":
        return extract_python_calls(file_path, symbols, all_symbols)

    # Pour C/C++/JS, utiliser regex
    return extract_calls_regex(file_path, symbols, all_symbols)


def extract_calls_regex(
    file_path: str,
    symbols: list,
    all_symbols: dict[str, int]
) -> list[dict[str, Any]]:
    """
    Extrait les appels de fonction avec regex (pour C/C++/JS).

    Args:
        file_path: Chemin du fichier source
        symbols: Symboles définis dans ce fichier
        all_symbols: Dict {symbol_name: symbol_id} de tous les symboles connus

    Returns:
        Liste de dict avec: caller, callee, line
    """
    try:
        content = Path(file_path).read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        logger.warning(f"Cannot read {file_path}: {e}")
        return []

    calls = []
    lines = content.split("\n")

    # Pattern pour détecter les appels de fonction : name(
    call_pattern = re.compile(r'\b([a-zA-Z_]\w*)\s*\(')

    # Mots-clés à ignorer (pas des appels de fonction)
    keywords = {
        # C/C++
        "if", "for", "while", "switch", "catch", "sizeof", "typeof", "alignof",
        "return", "else", "do", "case", "default", "break", "continue", "goto",
        "struct", "class", "enum", "union", "typedef", "define", "ifdef", "ifndef",
        "include", "pragma", "static_cast", "dynamic_cast", "const_cast", "reinterpret_cast",
        # Python (au cas où)
        "elif", "except", "with", "assert", "print", "lambda", "yield", "async", "await",
        # JS
        "function", "const", "let", "var", "new", "delete", "instanceof", "typeof",
    }

    # Construire l'index des fonctions locales avec leurs plages
    local_functions = {}
    for sym in symbols:
        if hasattr(sym, 'kind'):
            kind = sym.kind
            name = sym.name
            line_start = sym.line_start
            line_end = sym.line_end
        else:
            kind = sym.get('kind', '')
            name = sym.get('name', '')
            line_start = sym.get('line_start')
            line_end = sym.get('line_end')

        if kind in ("function", "method") and line_start:
            local_functions[name] = {
                "start": line_start,
                "end": line_end or line_start + 200,
            }

    # État pour ignorer les commentaires multi-lignes
    in_block_comment = False

    for line_num, line in enumerate(lines, 1):
        stripped = line.strip()

        # Gestion des commentaires bloc /* */
        if "/*" in line:
            in_block_comment = True
        if "*/" in line:
            in_block_comment = False
            continue
        if in_block_comment:
            continue

        # Ignorer les commentaires ligne et preprocessor
        if stripped.startswith("//") or stripped.startswith("/*"):
            continue
        if stripped.startswith("#") and not stripped.startswith("#include"):
            continue

        # Chercher les appels dans la ligne
        for match in call_pattern.finditer(line):
            callee_name = match.group(1)

            # Ignorer les mots-clés
            if callee_name in keywords:
                continue

            # Vérifier que le callee est un symbole connu
            if callee_name not in all_symbols and callee_name not in local_functions:
                continue

            # Trouver le caller (fonction qui contient cette ligne)
            caller_name = None
            for func_name, func_info in local_functions.items():
                if func_info["start"] <= line_num <= func_info["end"]:
                    caller_name = func_name
                    break

            # Si on a un caller et ce n'est pas un auto-appel
            if caller_name and caller_name != callee_name:
                calls.append({
                    "caller": caller_name,
                    "callee": callee_name,
                    "line": line_num,
                })

    return calls


# =============================================================================
# MAIN INDEXER CLASS
# =============================================================================

class CodeIndexer:
    """
    Indexeur principal de code source pour AgentDB.

    Orchestre le parsing des fichiers, l'extraction des symboles et relations,
    le calcul des métriques, et l'insertion dans la base de données.

    Attributes:
        db: Instance de DatabaseManager
        config: Configuration de l'indexeur
        ctags_available: True si ctags est disponible
        ctags_path: Chemin vers l'exécutable ctags
    """

    def __init__(
        self,
        db: DatabaseManager,
        config: Optional[IndexerConfig | dict] = None
    ) -> None:
        """
        Initialise l'indexeur.

        Args:
            db: Instance de DatabaseManager connectée
            config: Configuration (IndexerConfig, dict, ou None pour défauts)
        """
        self.db = db

        # Gérer la config
        if config is None:
            self.config = IndexerConfig()
        elif isinstance(config, dict):
            self.config = IndexerConfig.from_dict(config)
        else:
            self.config = config

        # Repositories
        self.files = FileRepository(db)
        self.symbols = SymbolRepository(db)
        self.relations = RelationRepository(db)
        self.file_relations = FileRelationRepository(db)

        # Vérifier ctags
        self.ctags_available, self.ctags_path = check_ctags_available(
            self.config.ctags_path
        )
        if not self.ctags_available:
            logger.warning(f"ctags not available: {self.ctags_path}")
        else:
            logger.info(f"Using ctags at: {self.ctags_path}")

        # Cache des symboles pour les relations
        self._symbol_cache: dict[str, int] = {}

    def _refresh_symbol_cache(self) -> None:
        """Rafraîchit le cache des symboles."""
        rows = self.db.fetch_all("SELECT id, name FROM symbols")
        self._symbol_cache = {r["name"]: r["id"] for r in rows}

    # -------------------------------------------------------------------------
    # PUBLIC API
    # -------------------------------------------------------------------------

    def index_file(self, file_path: str) -> IndexResult:
        """
        Indexe un seul fichier.

        Args:
            file_path: Chemin du fichier (relatif à project_root)

        Returns:
            IndexResult avec les détails de l'indexation

        Example:
            >>> result = indexer.index_file("src/main.c")
            >>> if result.success:
            ...     print(f"Indexed {result.symbols_count} symbols")
            ... else:
            ...     print(f"Errors: {result.errors}")
        """
        start_time = time.perf_counter()
        result = IndexResult(file_path=file_path)

        # Résoudre le chemin complet
        full_path = self.config.project_root / file_path

        try:
            # Vérifier que le fichier existe
            if not full_path.exists():
                result.errors.append(f"File not found: {file_path}")
                return result

            # Vérifier si le fichier doit être indexé
            if not self._should_index(full_path):
                result.warnings.append(f"File excluded by patterns: {file_path}")
                return result

            # Lire le contenu
            try:
                content = full_path.read_text(encoding="utf-8", errors="replace")
            except Exception as e:
                result.errors.append(f"Cannot read file: {e}")
                return result

            # Détecter le langage
            language = self._detect_language(full_path)
            if not language:
                result.warnings.append(f"Unknown language for {file_path}")

            # Calculer les métriques de lignes
            line_counts = count_lines(str(full_path), language)

            # Calculer la complexité
            complexity = calculate_complexity(str(full_path), language)

            # Calculer le hash
            content_hash = hashlib.sha256(content.encode()).hexdigest()

            # Créer ou mettre à jour l'entrée fichier
            existing = self.files.find_by_path(file_path)

            file_obj = File(
                id=existing.id if existing else None,
                path=file_path,
                filename=full_path.name,
                extension=full_path.suffix,
                module=self._detect_module(full_path),
                file_type=self._detect_file_type(full_path),
                language=language,
                is_critical=self._is_critical_path(file_path),
                security_sensitive=self._is_security_sensitive(file_path, content),
                lines_total=line_counts["total"],
                lines_code=line_counts["code"],
                lines_comment=line_counts["comment"],
                lines_blank=line_counts["blank"],
                complexity_sum=complexity["sum"],
                complexity_avg=complexity["avg"],
                complexity_max=complexity["max"],
                content_hash=content_hash,
                indexed_at=datetime.now().isoformat(),
            )

            if existing:
                # Supprimer les anciens symboles et relations
                self._delete_file_symbols(existing.id)
                file_id = self.files.update(file_obj)
            else:
                file_id = self.files.create(file_obj)

            result.file_id = file_id

            # Extraire les symboles avec ctags (pour C/C++) ou AST (pour Python)
            symbols = []
            if language in ("c", "cpp") and self.ctags_available:
                try:
                    tags = run_ctags(str(full_path), self.ctags_path, language=language)
                    symbols = ctags_to_symbols(tags, file_id, file_content=content)
                except Exception as e:
                    result.warnings.append(f"ctags failed: {e}")
                    logger.warning(f"ctags failed for {file_path}: {e}")
            elif language == "python":
                symbols = self._extract_python_symbols(full_path, file_id)
            elif language == "javascript" and self.ctags_available:
                # Fallback ctags pour JavaScript/TypeScript
                try:
                    tags = run_ctags(str(full_path), self.ctags_path, language=language)
                    symbols = ctags_to_symbols(tags, file_id, file_content=content)
                except Exception as e:
                    result.warnings.append(f"ctags failed for JS: {e}")

            # Insérer les symboles
            for sym in symbols:
                sym.file_id = file_id
                sym_id = self.symbols.create(sym)
                sym.id = sym_id

            result.symbols_count = len(symbols)

            # Extraire les includes/imports
            includes = extract_includes(str(full_path), language)
            file_relations = []

            for inc in includes:
                # Essayer de résoudre le fichier inclus
                target_file = self.files.find_by_path(inc["path"])
                if target_file:
                    fr = FileRelation(
                        source_file_id=file_id,
                        target_file_id=target_file.id,
                        relation_type="includes" if language in ("c", "cpp") else "imports",
                        line_number=inc["line"],
                    )
                    file_relations.append(fr)

            # Insérer les relations de fichiers
            for fr in file_relations:
                self.file_relations.create(fr)

            # Extraire les appels (après refresh du cache)
            self._refresh_symbol_cache()
            calls = extract_calls(str(full_path), symbols, self._symbol_cache)

            relations_count = 0
            for call in calls:
                caller_id = self._symbol_cache.get(call["caller"])
                callee_id = self._symbol_cache.get(call["callee"])

                if caller_id and callee_id:
                    rel = Relation(
                        source_id=caller_id,
                        target_id=callee_id,
                        relation_type="calls",
                        location_file_id=file_id,
                        location_line=call["line"],
                    )
                    self.relations.create(rel)
                    relations_count += 1

            result.relations_count = relations_count + len(file_relations)

            # Log le temps
            duration = (time.perf_counter() - start_time) * 1000
            result.duration_ms = round(duration, 2)

            logger.info(
                f"Indexed {file_path}: {result.symbols_count} symbols, "
                f"{result.relations_count} relations in {result.duration_ms:.1f}ms"
            )

        except Exception as e:
            result.errors.append(f"Unexpected error: {e}")
            logger.error(f"Failed to index {file_path}: {e}", exc_info=True)

        result.duration_ms = round((time.perf_counter() - start_time) * 1000, 2)
        return result

    def index_directory(
        self,
        dir_path: str = ".",
        recursive: bool = True
    ) -> list[IndexResult]:
        """
        Indexe tous les fichiers d'un répertoire.

        Args:
            dir_path: Chemin du répertoire (relatif à project_root)
            recursive: Inclure les sous-répertoires

        Returns:
            Liste de IndexResult pour chaque fichier

        Example:
            >>> results = indexer.index_directory("src/", recursive=True)
            >>> success = sum(1 for r in results if r.success)
            >>> print(f"Indexed {success}/{len(results)} files")
        """
        results = []
        full_dir = self.config.project_root / dir_path

        if not full_dir.exists():
            logger.error(f"Directory not found: {dir_path}")
            return results

        # Collecter les fichiers
        if recursive:
            files = list(full_dir.rglob("*"))
        else:
            files = list(full_dir.glob("*"))

        # Filtrer les fichiers
        files = [f for f in files if f.is_file() and self._should_index(f)]

        logger.info(f"Indexing {len(files)} files from {dir_path}")

        for file_path in files:
            # Convertir en chemin relatif
            try:
                rel_path = file_path.relative_to(self.config.project_root)
            except ValueError:
                rel_path = file_path

            result = self.index_file(str(rel_path))
            results.append(result)

        # Résumé
        success = sum(1 for r in results if r.success)
        total_symbols = sum(r.symbols_count for r in results)
        total_relations = sum(r.relations_count for r in results)

        logger.info(
            f"Indexing complete: {success}/{len(results)} files, "
            f"{total_symbols} symbols, {total_relations} relations"
        )

        return results

    def reindex_files(self, file_paths: list[str]) -> list[IndexResult]:
        """
        Réindexe une liste de fichiers (mise à jour incrémentale).

        Supprime les anciens symboles/relations et réindexe.

        Args:
            file_paths: Liste des chemins de fichiers

        Returns:
            Liste de IndexResult

        Example:
            >>> # Après modification de fichiers
            >>> modified = ["src/main.c", "src/utils.c"]
            >>> results = indexer.reindex_files(modified)
        """
        results = []

        logger.info(f"Reindexing {len(file_paths)} files")

        for file_path in file_paths:
            # Supprimer l'ancien index
            existing = self.files.find_by_path(file_path)
            if existing:
                self._delete_file_symbols(existing.id)

            # Réindexer
            result = self.index_file(file_path)
            results.append(result)

        return results

    # -------------------------------------------------------------------------
    # PRIVATE HELPERS
    # -------------------------------------------------------------------------

    def _should_index(self, file_path: Path) -> bool:
        """Vérifie si un fichier doit être indexé."""
        # Vérifier l'extension
        ext = file_path.suffix.lower()
        valid_ext = False
        for exts in self.config.extensions.values():
            if ext in exts:
                valid_ext = True
                break

        if not valid_ext:
            return False

        # Vérifier les exclusions
        rel_path = str(file_path)
        try:
            rel_path = str(file_path.relative_to(self.config.project_root))
        except ValueError:
            pass

        for pattern in self.config.exclude_patterns:
            if fnmatch.fnmatch(rel_path, pattern):
                return False

        return True

    def _detect_language(self, file_path: Path) -> Optional[str]:
        """Détecte le langage depuis l'extension."""
        ext = file_path.suffix.lower()
        for lang, exts in self.config.extensions.items():
            if ext in exts:
                return lang
        return None

    def _detect_module(self, file_path: Path) -> Optional[str]:
        """Déduit le module depuis le chemin."""
        try:
            rel = file_path.relative_to(self.config.project_root)
        except ValueError:
            rel = file_path

        parts = rel.parts
        # Ignorer "src" et le fichier lui-même
        if len(parts) >= 2:
            if parts[0] in ("src", "lib", "source"):
                if len(parts) >= 3:
                    return parts[1]
            return parts[0]
        return None

    def _detect_file_type(self, file_path: Path) -> str:
        """Détermine le type de fichier."""
        name = file_path.name.lower()
        ext = file_path.suffix.lower()

        # Headers
        if ext in (".h", ".hpp", ".hh", ".hxx"):
            return "header"

        # Tests
        if "test" in name or "_test" in name or "spec" in name:
            return "test"

        # Config
        if ext in (".json", ".yaml", ".yml", ".toml", ".ini", ".cfg"):
            return "config"

        # Documentation
        if ext in (".md", ".rst", ".txt"):
            return "doc"

        return "source"

    def _is_critical_path(self, file_path: str) -> bool:
        """Vérifie si le fichier est dans un chemin critique."""
        for pattern in self.config.critical_paths:
            if fnmatch.fnmatch(file_path, pattern):
                return True
        return False

    def _is_security_sensitive(self, file_path: str, content: str) -> bool:
        """Vérifie si le fichier est sensible (sécurité)."""
        # Patterns dans le nom
        sensitive_patterns = ["password", "secret", "token", "key", "crypt", "auth"]
        name_lower = file_path.lower()
        for pattern in sensitive_patterns:
            if pattern in name_lower:
                return True

        # Patterns dans le contenu (simplifié)
        sensitive_content = ["private_key", "api_key", "secret_key", "password"]
        content_lower = content.lower()
        for pattern in sensitive_content:
            if pattern in content_lower:
                return True

        return False

    def _delete_file_symbols(self, file_id: int) -> None:
        """Supprime tous les symboles et relations d'un fichier."""
        # Les relations seront supprimées en cascade grâce aux FK
        self.db.execute(
            "DELETE FROM symbols WHERE file_id = ?",
            (file_id,)
        )
        self.db.execute(
            "DELETE FROM file_relations WHERE source_file_id = ? OR target_file_id = ?",
            (file_id, file_id)
        )

    def _extract_python_symbols(self, file_path: Path, file_id: int) -> list[Symbol]:
        """
        Extrait les symboles d'un fichier Python avec ast.

        Détecte :
        - Fonctions (def) avec signature, visibilité, complexité
        - Classes avec bases d'héritage
        - Méthodes dans les classes (kind="method")
        - Propriétés (@property)
        """
        import ast

        try:
            content = file_path.read_text(encoding="utf-8")
            tree = ast.parse(content, filename=str(file_path))
        except SyntaxError as e:
            logger.warning(f"Python syntax error in {file_path}: {e}")
            return []
        except Exception as e:
            logger.warning(f"Cannot parse {file_path}: {e}")
            return []

        symbols = []
        lines = content.split("\n")

        def get_visibility(name: str, decorators: list) -> str:
            """Détermine la visibilité d'un symbole Python."""
            # Convention Python: _name = private, __name = very private
            if name.startswith("__") and not name.endswith("__"):
                return "private"
            elif name.startswith("_"):
                return "protected"
            return "public"

        def has_decorator(decorators: list, name: str) -> bool:
            """Vérifie si un décorateur est présent."""
            for d in decorators:
                if isinstance(d, ast.Name) and d.id == name:
                    return True
                elif isinstance(d, ast.Attribute) and d.attr == name:
                    return True
            return False

        def calculate_python_complexity(node) -> int:
            """Calcule la complexité cyclomatique d'une fonction Python."""
            complexity = 1

            for child in ast.walk(node):
                if isinstance(child, ast.If):
                    complexity += 1
                elif isinstance(child, ast.For):
                    complexity += 1
                elif isinstance(child, ast.While):
                    complexity += 1
                elif isinstance(child, ast.ExceptHandler):
                    complexity += 1
                elif isinstance(child, ast.With):
                    complexity += 1
                elif isinstance(child, ast.Assert):
                    complexity += 1
                elif isinstance(child, ast.comprehension):
                    complexity += 1
                elif isinstance(child, ast.BoolOp):
                    # and/or ajoutent de la complexité
                    complexity += len(child.values) - 1
                elif isinstance(child, ast.IfExp):  # ternaire
                    complexity += 1

            return complexity

        def extract_return_type(node) -> Optional[str]:
            """Extrait le type de retour d'une fonction."""
            if node.returns:
                try:
                    return ast.unparse(node.returns)
                except Exception:
                    return None
            return None

        # Parcourir l'AST de manière structurée
        for node in ast.iter_child_nodes(tree):
            # Fonctions de niveau module
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                is_async = isinstance(node, ast.AsyncFunctionDef)
                visibility = get_visibility(node.name, node.decorator_list)
                is_static = has_decorator(node.decorator_list, "staticmethod")

                sym = Symbol(
                    file_id=file_id,
                    name=node.name,
                    kind="function",
                    line_start=node.lineno,
                    line_end=node.end_lineno,
                    signature=self._get_python_signature(node),
                    return_type=extract_return_type(node),
                    visibility=visibility,
                    is_static=is_static,
                    is_exported=visibility == "public",
                    complexity=calculate_python_complexity(node),
                    doc_comment=ast.get_docstring(node),
                    has_doc=ast.get_docstring(node) is not None,
                )
                symbols.append(sym)

            # Classes
            elif isinstance(node, ast.ClassDef):
                bases = []
                for base in node.bases:
                    try:
                        bases.append(ast.unparse(base))
                    except Exception:
                        if isinstance(base, ast.Name):
                            bases.append(base.id)

                class_visibility = get_visibility(node.name, node.decorator_list)

                class_sym = Symbol(
                    file_id=file_id,
                    name=node.name,
                    kind="class",
                    line_start=node.lineno,
                    line_end=node.end_lineno,
                    visibility=class_visibility,
                    is_exported=class_visibility == "public",
                    base_classes_json=json.dumps(bases) if bases else None,
                    doc_comment=ast.get_docstring(node),
                    has_doc=ast.get_docstring(node) is not None,
                )
                symbols.append(class_sym)

                # Extraire les méthodes de la classe
                for class_node in ast.iter_child_nodes(node):
                    if isinstance(class_node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                        method_name = class_node.name
                        method_visibility = get_visibility(method_name, class_node.decorator_list)

                        # Déterminer le type de méthode
                        is_static = has_decorator(class_node.decorator_list, "staticmethod")
                        is_classmethod = has_decorator(class_node.decorator_list, "classmethod")
                        is_property = has_decorator(class_node.decorator_list, "property")

                        kind = "method"
                        if is_property:
                            kind = "property"

                        method_sym = Symbol(
                            file_id=file_id,
                            name=method_name,
                            qualified_name=f"{node.name}.{method_name}",
                            kind=kind,
                            line_start=class_node.lineno,
                            line_end=class_node.end_lineno,
                            signature=self._get_python_signature(class_node),
                            return_type=extract_return_type(class_node),
                            visibility=method_visibility,
                            is_static=is_static,
                            is_exported=method_visibility == "public" and class_visibility == "public",
                            complexity=calculate_python_complexity(class_node),
                            doc_comment=ast.get_docstring(class_node),
                            has_doc=ast.get_docstring(class_node) is not None,
                        )
                        symbols.append(method_sym)

        return symbols

    def _get_python_signature(self, node) -> Optional[str]:
        """Construit la signature complète d'une fonction Python."""
        import ast

        try:
            args_parts = []

            # Arguments positionnels normaux
            defaults_offset = len(node.args.args) - len(node.args.defaults)
            for i, arg in enumerate(node.args.args):
                arg_str = arg.arg
                if arg.annotation:
                    arg_str += f": {ast.unparse(arg.annotation)}"
                # Ajouter la valeur par défaut si présente
                default_idx = i - defaults_offset
                if default_idx >= 0 and default_idx < len(node.args.defaults):
                    try:
                        arg_str += f" = {ast.unparse(node.args.defaults[default_idx])}"
                    except Exception:
                        arg_str += " = ..."
                args_parts.append(arg_str)

            # *args
            if node.args.vararg:
                vararg = f"*{node.args.vararg.arg}"
                if node.args.vararg.annotation:
                    vararg += f": {ast.unparse(node.args.vararg.annotation)}"
                args_parts.append(vararg)

            # keyword-only args
            for i, arg in enumerate(node.args.kwonlyargs):
                arg_str = arg.arg
                if arg.annotation:
                    arg_str += f": {ast.unparse(arg.annotation)}"
                if i < len(node.args.kw_defaults) and node.args.kw_defaults[i]:
                    try:
                        arg_str += f" = {ast.unparse(node.args.kw_defaults[i])}"
                    except Exception:
                        arg_str += " = ..."
                args_parts.append(arg_str)

            # **kwargs
            if node.args.kwarg:
                kwarg = f"**{node.args.kwarg.arg}"
                if node.args.kwarg.annotation:
                    kwarg += f": {ast.unparse(node.args.kwarg.annotation)}"
                args_parts.append(kwarg)

            # Construire la signature
            prefix = "async def" if isinstance(node, ast.AsyncFunctionDef) else "def"
            sig = f"{prefix} {node.name}({', '.join(args_parts)})"
            if node.returns:
                sig += f" -> {ast.unparse(node.returns)}"
            return sig
        except Exception as e:
            logger.debug(f"Could not build signature for {node.name}: {e}")
            return f"def {node.name}(...)"


# =============================================================================
# EXPORTS
# =============================================================================

__all__ = [
    # Classes
    "CodeIndexer",
    "IndexerConfig",
    "IndexResult",
    # Functions
    "run_ctags",
    "parse_ctags_output",
    "ctags_to_symbols",
    "count_lines",
    "calculate_complexity",
    "extract_includes",
    "extract_calls",
    "check_ctags_available",
]
