#!/usr/bin/env python3
"""
AgentDB Bootstrap Script - Initialisation de la base de données.

Ce script exécute les 9 étapes du bootstrap :
1. Créer la structure (dossiers)
2. Initialiser le schéma SQL
3. Scanner les fichiers
4. Indexer les symboles et relations
5. Calculer les métriques
6. Analyser l'activité Git
7. Marquer les fichiers critiques
8. Importer les patterns par défaut
9. Vérifier l'intégrité

Usage:
    python -m scripts.bootstrap [--project-root PATH]
    python .claude/scripts/bootstrap.py [--project-root PATH]
"""

from __future__ import annotations

import argparse
import fnmatch
import hashlib
import json
import logging
import os
import re
import shutil
import sqlite3
import subprocess
import sys
import time
import warnings
from dataclasses import dataclass, field
from datetime import datetime, timedelta
from pathlib import Path
from typing import Any, Optional

# =============================================================================
# CONFIGURATION
# =============================================================================

# Couleurs pour le terminal
class Colors:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    MAGENTA = "\033[95m"
    CYAN = "\033[96m"


# Désactiver les couleurs si pas de TTY
if not sys.stdout.isatty():
    for attr in dir(Colors):
        if not attr.startswith("_"):
            setattr(Colors, attr, "")


@dataclass
class BootstrapConfig:
    """Configuration du bootstrap."""
    project_root: Path
    claude_dir: Path
    agentdb_dir: Path
    db_path: Path
    schema_path: Path
    config_path: Path
    logs_dir: Path

    # Extensions par langage
    extensions: dict[str, list[str]] = field(default_factory=lambda: {
        "c": [".c", ".h"],
        "cpp": [".cpp", ".hpp", ".cc", ".hh", ".cxx", ".hxx"],
        "python": [".py", ".pyi"],
        "javascript": [".js", ".jsx", ".mjs"],
        "typescript": [".ts", ".tsx"],
        "rust": [".rs"],
        "go": [".go"],
    })

    # Patterns à exclure
    exclude_patterns: list[str] = field(default_factory=lambda: [
        # Build outputs
        "build/**", "buildLinux/**", "buildWindows/**", "buildMac/**",
        "dist/**", "out/**", "target/**", "artifacts/**",
        "*.o", "*.obj", "*.exe", "*.dll", "*.so", "*.a", "*.lib",
        # Dependencies
        "third_party/**", "external/**", "deps/**",
        "vendor/**", "node_modules/**", ".venv/**", "venv/**",
        # Caches
        ".cache/**", ".direnv/**", ".ccache/**",
        "__pycache__/**", "*.pyc", "*.egg-info/**",
        ".pytest_cache/**", "coverage/**", ".coverage",
        # IDE/VCS
        ".git/**", ".svn/**", ".idea/**", ".vscode/**",
        "*.swp", "*~",
        # Generated/minified
        "**/*.min.js", "**/*.min.css", "**/*.generated.*",
        "**/generated/**",
        # Project specific
        ".claude/agentdb/**", ".claude/logs/**",
        "assets/**", "logs/**",
    ])

    # Chemins critiques
    critical_paths: list[str] = field(default_factory=lambda: [
        "**/security/**", "**/auth/**", "**/authentication/**",
        "**/crypto/**", "**/encryption/**",
        "**/*password*", "**/*secret*", "**/*token*",
        "**/*credential*", "**/*key*",
        "**/main.*", "**/init.*", "**/bootstrap.*",
        "**/migration*", "**/schema*",
    ])

    # Chemins haute importance
    high_importance_paths: list[str] = field(default_factory=lambda: [
        "**/core/**", "**/kernel/**", "**/api/**",
        "**/services/**", "**/models/**", "**/handlers/**",
    ])

    @classmethod
    def from_project_root(cls, project_root: Path) -> "BootstrapConfig":
        """Crée une configuration à partir de la racine du projet."""
        claude_dir = project_root / ".claude"
        return cls(
            project_root=project_root,
            claude_dir=claude_dir,
            agentdb_dir=claude_dir / "agentdb",
            db_path=claude_dir / "agentdb" / "db.sqlite",
            schema_path=claude_dir / "agentdb" / "schema.sql",
            config_path=claude_dir / "config" / "agentdb.yaml",
            logs_dir=claude_dir / "logs",
        )


@dataclass
class BootstrapStats:
    """Statistiques du bootstrap."""
    files_scanned: int = 0
    files_indexed: int = 0
    files_skipped: int = 0
    symbols_indexed: int = 0
    relations_indexed: int = 0
    file_relations_indexed: int = 0
    errors: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)
    start_time: float = 0
    end_time: float = 0

    @property
    def duration_seconds(self) -> float:
        return self.end_time - self.start_time


# =============================================================================
# PROGRESS BAR
# =============================================================================

class ProgressBar:
    """Barre de progression simple."""

    def __init__(self, total: int, description: str = "", width: int = 40):
        self.total = max(total, 1)
        self.current = 0
        self.description = description
        self.width = width
        self.start_time = time.time()

    def update(self, n: int = 1) -> None:
        """Met à jour la progression."""
        self.current = min(self.current + n, self.total)
        self._render()

    def _render(self) -> None:
        """Affiche la barre de progression."""
        if not sys.stdout.isatty():
            return

        percent = self.current / self.total
        filled = int(self.width * percent)
        bar = "█" * filled + "░" * (self.width - filled)

        elapsed = time.time() - self.start_time
        if self.current > 0:
            eta = (elapsed / self.current) * (self.total - self.current)
            eta_str = f"ETA: {eta:.0f}s"
        else:
            eta_str = ""

        line = f"\r{Colors.CYAN}{self.description}{Colors.RESET} |{bar}| {self.current}/{self.total} ({percent:.0%}) {eta_str}  "
        sys.stdout.write(line)
        sys.stdout.flush()

    def finish(self) -> None:
        """Termine la barre de progression."""
        self.current = self.total
        self._render()
        if sys.stdout.isatty():
            print()


# =============================================================================
# LOGGER
# =============================================================================

def setup_logging(logs_dir: Path) -> logging.Logger:
    """Configure le logging."""
    logs_dir.mkdir(parents=True, exist_ok=True)

    logger = logging.getLogger("agentdb.bootstrap")
    logger.setLevel(logging.DEBUG)

    # File handler
    log_file = logs_dir / "bootstrap.log"
    fh = logging.FileHandler(log_file, mode="w", encoding="utf-8")
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(logging.Formatter(
        "%(asctime)s - %(levelname)s - %(message)s"
    ))
    logger.addHandler(fh)

    # Console handler (warnings and errors only)
    ch = logging.StreamHandler()
    ch.setLevel(logging.WARNING)
    ch.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
    logger.addHandler(ch)

    return logger


# =============================================================================
# STEP 1: CREATE STRUCTURE
# =============================================================================

def step_1_create_structure(config: BootstrapConfig, logger: logging.Logger) -> bool:
    """Étape 1 : Créer la structure de dossiers."""
    print(f"\n{Colors.BOLD}Step 1/9:{Colors.RESET} Creating directory structure...")

    directories = [
        config.agentdb_dir,
        config.claude_dir / "mcp" / "agentdb",
        config.claude_dir / "config",
        config.logs_dir,
        config.claude_dir / "scripts",
    ]

    for d in directories:
        d.mkdir(parents=True, exist_ok=True)
        logger.debug(f"Created directory: {d}")

    # Créer __init__.py si nécessaire
    init_files = [
        config.claude_dir / "mcp" / "__init__.py",
        config.claude_dir / "mcp" / "agentdb" / "__init__.py",
        config.agentdb_dir / "__init__.py",
        config.claude_dir / "scripts" / "__init__.py",
    ]

    for init_file in init_files:
        if not init_file.exists():
            init_file.write_text('"""AgentDB module."""\n')
            logger.debug(f"Created: {init_file}")

    print(f"  {Colors.GREEN}✓{Colors.RESET} Directory structure created")
    return True


# =============================================================================
# STEP 2: INITIALIZE SCHEMA
# =============================================================================

def step_2_init_schema(config: BootstrapConfig, logger: logging.Logger) -> bool:
    """Étape 2 : Initialiser le schéma SQL."""
    print(f"\n{Colors.BOLD}Step 2/9:{Colors.RESET} Initializing database schema...")

    # Vérifier que le schéma existe
    if not config.schema_path.exists():
        logger.error(f"Schema file not found: {config.schema_path}")
        print(f"  {Colors.RED}✗{Colors.RESET} Schema file not found: {config.schema_path}")
        return False

    # Supprimer l'ancienne base si elle existe
    if config.db_path.exists():
        backup_path = config.db_path.with_suffix(".sqlite.bak")
        shutil.copy(config.db_path, backup_path)
        config.db_path.unlink()
        logger.info(f"Backed up existing database to {backup_path}")

    # Créer la nouvelle base
    try:
        conn = sqlite3.connect(str(config.db_path))
        schema_sql = config.schema_path.read_text(encoding="utf-8")
        conn.executescript(schema_sql)
        conn.commit()

        # Vérifier les tables créées
        cursor = conn.execute(
            "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name"
        )
        tables = [row[0] for row in cursor.fetchall()]

        conn.close()

        logger.info(f"Created database with tables: {tables}")
        print(f"  {Colors.GREEN}✓{Colors.RESET} Database created with {len(tables)} tables")
        return True

    except sqlite3.Error as e:
        logger.error(f"Database error: {e}")
        print(f"  {Colors.RED}✗{Colors.RESET} Database error: {e}")
        return False


# =============================================================================
# STEP 3: SCAN FILES
# =============================================================================

def should_exclude(path: str, patterns: list[str]) -> bool:
    """Vérifie si un chemin doit être exclu."""
    path_parts = path.split("/")

    for pattern in patterns:
        # Pattern simple sans ** : utiliser fnmatch
        if "**" not in pattern:
            if fnmatch.fnmatch(path, pattern):
                return True
            # Vérifier aussi le nom de fichier seul
            if fnmatch.fnmatch(path_parts[-1], pattern):
                return True
            continue

        # Pattern "dir/**" : exclure tout le dossier
        if pattern.endswith("/**"):
            prefix = pattern[:-3]
            if path.startswith(prefix + "/") or path == prefix:
                return True
            continue

        # Pattern "**/suffix" : vérifier la fin du chemin
        if pattern.startswith("**/"):
            suffix = pattern[3:]
            if fnmatch.fnmatch(path, suffix) or path.endswith("/" + suffix):
                return True
            # Vérifier chaque composant du chemin
            for i, part in enumerate(path_parts):
                subpath = "/".join(path_parts[i:])
                if fnmatch.fnmatch(subpath, suffix):
                    return True
            continue

        # Pattern complexe avec ** au milieu
        if fnmatch.fnmatch(path, pattern.replace("**", "*")):
            return True

    return False


def get_language(ext: str, extensions: dict[str, list[str]]) -> Optional[str]:
    """Détermine le langage à partir de l'extension."""
    for lang, exts in extensions.items():
        if ext.lower() in exts:
            return lang
    return None


def count_lines(file_path: Path) -> dict[str, int]:
    """Compte les lignes d'un fichier."""
    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
        lines = content.split("\n")

        total = len(lines)
        blank = sum(1 for line in lines if not line.strip())

        # Estimation simple des commentaires
        comment = 0
        in_block = False
        for line in lines:
            stripped = line.strip()
            if stripped.startswith("/*"):
                in_block = True
            if in_block:
                comment += 1
                if "*/" in stripped:
                    in_block = False
            elif stripped.startswith("//") or stripped.startswith("#"):
                comment += 1

        code = total - blank - comment
        return {
            "total": total,
            "code": max(0, code),
            "comment": comment,
            "blank": blank,
        }
    except Exception:
        return {"total": 0, "code": 0, "comment": 0, "blank": 0}


def get_content_hash(file_path: Path) -> str:
    """Calcule le hash MD5 du contenu."""
    try:
        content = file_path.read_bytes()
        return hashlib.md5(content).hexdigest()
    except Exception:
        return ""


def get_module_from_path(path: str, project_root: Path) -> str:
    """Détermine le module à partir du chemin."""
    rel_path = Path(path)
    parts = rel_path.parts

    # Le module est généralement le premier ou deuxième répertoire
    if len(parts) >= 2:
        if parts[0] in ("src", "lib", "app", "pkg"):
            return parts[1] if len(parts) > 1 else parts[0]
        return parts[0]
    return "root"


def step_3_scan_files(
    config: BootstrapConfig,
    logger: logging.Logger,
    stats: BootstrapStats
) -> list[dict[str, Any]]:
    """Étape 3 : Scanner les fichiers du projet."""
    print(f"\n{Colors.BOLD}Step 3/9:{Colors.RESET} Scanning files...")

    # Collecter toutes les extensions valides
    valid_extensions = set()
    for exts in config.extensions.values():
        valid_extensions.update(exts)

    # Scanner les fichiers
    files_to_index: list[dict[str, Any]] = []
    all_files = list(config.project_root.rglob("*"))

    progress = ProgressBar(len(all_files), "Scanning")

    for file_path in all_files:
        progress.update()

        if not file_path.is_file():
            continue

        # Chemin relatif
        try:
            rel_path = file_path.relative_to(config.project_root)
        except ValueError:
            continue

        rel_path_str = str(rel_path).replace("\\", "/")

        # Vérifier les exclusions
        if should_exclude(rel_path_str, config.exclude_patterns):
            stats.files_skipped += 1
            continue

        # Vérifier l'extension
        ext = file_path.suffix.lower()
        if ext not in valid_extensions:
            stats.files_skipped += 1
            continue

        # Récupérer les infos du fichier
        language = get_language(ext, config.extensions)
        line_counts = count_lines(file_path)
        content_hash = get_content_hash(file_path)
        module = get_module_from_path(rel_path_str, config.project_root)

        file_info = {
            "path": rel_path_str,
            "filename": file_path.name,
            "extension": ext,
            "module": module,
            "language": language,
            "lines_total": line_counts["total"],
            "lines_code": line_counts["code"],
            "lines_comment": line_counts["comment"],
            "lines_blank": line_counts["blank"],
            "content_hash": content_hash,
            "full_path": file_path,
        }

        files_to_index.append(file_info)
        stats.files_scanned += 1

        logger.debug(f"Scanned: {rel_path_str}")

    progress.finish()

    # Insérer dans la base
    if files_to_index:
        conn = sqlite3.connect(str(config.db_path))
        cursor = conn.cursor()

        for f in files_to_index:
            cursor.execute("""
                INSERT INTO files (
                    path, filename, extension, module, language,
                    lines_total, lines_code, lines_comment, lines_blank,
                    content_hash, indexed_at
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'))
            """, (
                f["path"], f["filename"], f["extension"], f["module"], f["language"],
                f["lines_total"], f["lines_code"], f["lines_comment"], f["lines_blank"],
                f["content_hash"],
            ))
            f["id"] = cursor.lastrowid

        conn.commit()
        conn.close()

    print(f"  {Colors.GREEN}✓{Colors.RESET} Scanned {stats.files_scanned} files ({stats.files_skipped} skipped)")
    return files_to_index


# =============================================================================
# STEP 4: INDEX SYMBOLS AND RELATIONS
# =============================================================================

def run_ctags(file_path: Path) -> list[dict[str, Any]]:
    """Exécute ctags sur un fichier et retourne les symboles."""
    try:
        result = subprocess.run(
            [
                "ctags", "--output-format=json", "--fields=*",
                "-o", "-", str(file_path)
            ],
            capture_output=True,
            text=True,
            timeout=30,
        )

        symbols = []
        for line in result.stdout.strip().split("\n"):
            if line:
                try:
                    tag = json.loads(line)
                    symbols.append({
                        "name": tag.get("name", ""),
                        "kind": tag.get("kind", "unknown"),
                        "line_start": tag.get("line", 0),
                        "signature": tag.get("signature", ""),
                        "scope": tag.get("scope", ""),
                        "scopeKind": tag.get("scopeKind", ""),
                    })
                except json.JSONDecodeError:
                    pass

        return symbols
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return []


def parse_python_file(file_path: Path) -> list[dict[str, Any]]:
    """
    Parse un fichier Python avec ast.

    Extrait :
    - Fonctions de niveau module
    - Classes avec bases d'héritage
    - Méthodes dans les classes
    - Visibilité (public/protected/private)
    - Complexité cyclomatique
    """
    import ast

    def get_visibility(name: str) -> str:
        """Détermine la visibilité selon les conventions Python."""
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

    def calculate_complexity(node) -> int:
        """Calcule la complexité cyclomatique."""
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
            elif isinstance(child, ast.comprehension):
                complexity += 1
            elif isinstance(child, ast.BoolOp):
                complexity += len(child.values) - 1
            elif isinstance(child, ast.IfExp):
                complexity += 1
        return complexity

    def build_signature(node) -> str:
        """Construit la signature d'une fonction."""
        try:
            args = []
            for arg in node.args.args:
                arg_str = arg.arg
                if arg.annotation:
                    arg_str += f": {ast.unparse(arg.annotation)}"
                args.append(arg_str)

            sig = f"({', '.join(args)})"
            if node.returns:
                sig += f" -> {ast.unparse(node.returns)}"
            return sig
        except Exception:
            return "()"

    try:
        content = file_path.read_text(encoding="utf-8")
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=SyntaxWarning)
            tree = ast.parse(content)

        symbols = []

        # Parcourir de manière structurée (pas ast.walk pour éviter les doublons)
        for node in ast.iter_child_nodes(tree):
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                # Fonction de niveau module
                visibility = get_visibility(node.name)
                is_static = has_decorator(node.decorator_list, "staticmethod")

                symbols.append({
                    "name": node.name,
                    "kind": "function",
                    "line_start": node.lineno,
                    "line_end": node.end_lineno,
                    "signature": build_signature(node),
                    "visibility": visibility,
                    "complexity": calculate_complexity(node),
                    "is_static": is_static,
                    "doc_comment": ast.get_docstring(node),
                })

            elif isinstance(node, ast.ClassDef):
                # Classe
                bases = []
                for base in node.bases:
                    try:
                        bases.append(ast.unparse(base))
                    except Exception:
                        if isinstance(base, ast.Name):
                            bases.append(base.id)

                class_visibility = get_visibility(node.name)

                symbols.append({
                    "name": node.name,
                    "kind": "class",
                    "line_start": node.lineno,
                    "line_end": node.end_lineno,
                    "visibility": class_visibility,
                    "base_classes": bases,
                    "doc_comment": ast.get_docstring(node),
                })

                # Méthodes de la classe
                for class_node in ast.iter_child_nodes(node):
                    if isinstance(class_node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                        method_name = class_node.name
                        method_visibility = get_visibility(method_name)
                        is_static = has_decorator(class_node.decorator_list, "staticmethod")
                        is_property = has_decorator(class_node.decorator_list, "property")

                        kind = "property" if is_property else "method"

                        symbols.append({
                            "name": method_name,
                            "qualified_name": f"{node.name}.{method_name}",
                            "kind": kind,
                            "line_start": class_node.lineno,
                            "line_end": class_node.end_lineno,
                            "signature": build_signature(class_node),
                            "visibility": method_visibility,
                            "complexity": calculate_complexity(class_node),
                            "is_static": is_static,
                            "doc_comment": ast.get_docstring(class_node),
                        })

        return symbols
    except Exception:
        return []


def extract_includes(file_path: Path, language: str) -> list[dict[str, str]]:
    """Extrait les includes/imports d'un fichier."""
    includes = []

    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
        lines = content.split("\n")

        for i, line in enumerate(lines, 1):
            line = line.strip()

            if language in ("c", "cpp"):
                # #include <...> ou #include "..."
                match = re.match(r'#include\s*[<"]([^>"]+)[>"]', line)
                if match:
                    includes.append({
                        "target": match.group(1),
                        "line": i,
                        "type": "include",
                    })

            elif language == "python":
                # import ... ou from ... import ...
                match = re.match(r'(?:from\s+(\S+)\s+)?import\s+(.+)', line)
                if match:
                    module = match.group(1) or match.group(2).split(",")[0].strip()
                    includes.append({
                        "target": module.split()[0],
                        "line": i,
                        "type": "import",
                    })
    except Exception:
        pass

    return includes


def step_4_index_symbols(
    config: BootstrapConfig,
    logger: logging.Logger,
    stats: BootstrapStats,
    files: list[dict[str, Any]]
) -> None:
    """Étape 4 : Indexer les symboles et relations."""
    print(f"\n{Colors.BOLD}Step 4/9:{Colors.RESET} Indexing symbols and relations...")

    # Vérifier si ctags est disponible
    ctags_available = shutil.which("ctags") is not None
    if not ctags_available:
        logger.warning("ctags not found, using fallback parsing")
        stats.warnings.append("ctags not found, using limited parsing")

    conn = sqlite3.connect(str(config.db_path))
    cursor = conn.cursor()

    progress = ProgressBar(len(files), "Indexing")

    # Index de tous les symboles pour les relations
    all_symbols: dict[str, int] = {}

    for file_info in files:
        progress.update()

        file_id = file_info["id"]
        file_path = file_info["full_path"]
        language = file_info.get("language", "")

        # Parser les symboles
        if language == "python":
            symbols = parse_python_file(file_path)
        elif ctags_available and language in ("c", "cpp"):
            symbols = run_ctags(file_path)
        else:
            symbols = []

        # Insérer les symboles avec tous les champs
        for sym in symbols:
            cursor.execute("""
                INSERT INTO symbols (
                    file_id, name, qualified_name, kind, line_start, line_end,
                    signature, visibility, complexity, is_static,
                    doc_comment, has_doc, base_classes_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                file_id,
                sym.get("name", ""),
                sym.get("qualified_name"),
                sym.get("kind", "unknown"),
                sym.get("line_start"),
                sym.get("line_end"),
                sym.get("signature", ""),
                sym.get("visibility", "public"),
                sym.get("complexity", 0),
                1 if sym.get("is_static") else 0,
                sym.get("doc_comment", ""),
                1 if sym.get("doc_comment") else 0,
                json.dumps(sym.get("base_classes")) if sym.get("base_classes") else None,
            ))

            sym_id = cursor.lastrowid
            stats.symbols_indexed += 1

            # Indexer pour les relations
            all_symbols[sym.get("name", "")] = sym_id

        # Extraire les includes/imports
        includes = extract_includes(file_path, language)
        for inc in includes:
            # Chercher le fichier cible
            target_pattern = f"%{inc['target']}%"
            cursor.execute(
                "SELECT id FROM files WHERE path LIKE ?",
                (target_pattern,)
            )
            target = cursor.fetchone()

            if target:
                cursor.execute("""
                    INSERT OR IGNORE INTO file_relations (
                        source_file_id, target_file_id, relation_type, line_number
                    ) VALUES (?, ?, 'includes', ?)
                """, (file_id, target[0], inc["line"]))
                stats.file_relations_indexed += 1

        logger.debug(f"Indexed {len(symbols)} symbols from {file_info['path']}")

    progress.finish()
    conn.commit()
    conn.close()

    stats.files_indexed = len(files)
    print(f"  {Colors.GREEN}✓{Colors.RESET} Indexed {stats.symbols_indexed} symbols, {stats.file_relations_indexed} file relations")


# =============================================================================
# STEP 4b: EXTRACT CALL RELATIONS
# =============================================================================

def extract_python_calls_for_bootstrap(
    file_path: Path,
    symbols: list[dict[str, Any]],
    all_symbols: dict[str, int]
) -> list[dict[str, Any]]:
    """
    Extrait les appels de fonction depuis un fichier Python en utilisant l'AST.

    Args:
        file_path: Chemin du fichier Python
        symbols: Symboles définis dans ce fichier
        all_symbols: Dict {symbol_name: symbol_id} de tous les symboles connus

    Returns:
        Liste de dict avec: caller, callee, line
    """
    import ast

    try:
        content = file_path.read_text(encoding="utf-8")
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=SyntaxWarning)
            tree = ast.parse(content, filename=str(file_path))
    except Exception as e:
        return []

    calls = []

    # Index des fonctions locales
    local_functions = set()
    for sym in symbols:
        if sym.get("kind") in ("function", "method"):
            local_functions.add(sym.get("name", ""))

    class CallVisitor(ast.NodeVisitor):
        def __init__(self):
            self.current_function = None

        def visit_FunctionDef(self, node):
            old_func = self.current_function
            self.current_function = node.name
            self.generic_visit(node)
            self.current_function = old_func

        def visit_AsyncFunctionDef(self, node):
            self.visit_FunctionDef(node)

        def visit_Call(self, node):
            callee_name = None

            if isinstance(node.func, ast.Name):
                callee_name = node.func.id
            elif isinstance(node.func, ast.Attribute):
                callee_name = node.func.attr

            if callee_name and self.current_function:
                # Vérifier que le callee est un symbole connu
                if callee_name in all_symbols or callee_name in local_functions:
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


def step_4b_extract_relations(
    config: BootstrapConfig,
    logger: logging.Logger,
    stats: BootstrapStats,
    files: list[dict[str, Any]]
) -> None:
    """Étape 4b : Extraire les relations d'appels entre symboles."""
    print(f"\n{Colors.BOLD}Step 4b/9:{Colors.RESET} Extracting call relations...")

    conn = sqlite3.connect(str(config.db_path))
    cursor = conn.cursor()

    # Construire l'index global des symboles: name -> id
    cursor.execute("SELECT id, name FROM symbols")
    all_symbols: dict[str, int] = {}
    for row in cursor.fetchall():
        all_symbols[row[1]] = row[0]

    logger.info(f"Loaded {len(all_symbols)} symbols for relation extraction")

    progress = ProgressBar(len(files), "Relations")
    relations_count = 0

    for file_info in files:
        progress.update()

        file_id = file_info["id"]
        file_path = file_info["full_path"]
        language = file_info.get("language", "")

        # Récupérer les symboles de ce fichier
        cursor.execute("""
            SELECT id, name, kind, line_start, line_end
            FROM symbols
            WHERE file_id = ?
        """, (file_id,))

        file_symbols = []
        for row in cursor.fetchall():
            file_symbols.append({
                "id": row[0],
                "name": row[1],
                "kind": row[2],
                "line_start": row[3],
                "line_end": row[4],
            })

        if not file_symbols:
            continue

        # Extraire les appels
        calls = []
        if language == "python":
            calls = extract_python_calls_for_bootstrap(file_path, file_symbols, all_symbols)
        elif language in ("c", "cpp", "javascript"):
            # Utiliser regex pour C/C++/JS
            calls = extract_calls_regex_for_bootstrap(file_path, file_symbols, all_symbols)

        # Insérer les relations
        for call in calls:
            caller_name = call["caller"]
            callee_name = call["callee"]
            line = call["line"]

            # Trouver les IDs
            caller_id = all_symbols.get(caller_name)
            callee_id = all_symbols.get(callee_name)

            if caller_id and callee_id and caller_id != callee_id:
                try:
                    cursor.execute("""
                        INSERT INTO relations (
                            source_id, target_id, relation_type,
                            location_file_id, location_line, count, is_direct
                        ) VALUES (?, ?, 'calls', ?, ?, 1, 1)
                    """, (caller_id, callee_id, file_id, line))
                    relations_count += 1
                except sqlite3.IntegrityError:
                    # Relation déjà existante, incrémenter le compteur
                    cursor.execute("""
                        UPDATE relations SET count = count + 1
                        WHERE source_id = ? AND target_id = ? AND relation_type = 'calls'
                    """, (caller_id, callee_id))

        logger.debug(f"Extracted {len(calls)} calls from {file_info['path']}")

    progress.finish()
    conn.commit()
    conn.close()

    stats.relations_indexed = relations_count
    print(f"  {Colors.GREEN}✓{Colors.RESET} Extracted {relations_count} call relations")


def extract_calls_regex_for_bootstrap(
    file_path: Path,
    symbols: list[dict[str, Any]],
    all_symbols: dict[str, int]
) -> list[dict[str, Any]]:
    """
    Extrait les appels de fonction avec regex (pour C/C++/JS).
    """
    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
    except Exception:
        return []

    calls = []
    lines = content.split("\n")

    call_pattern = re.compile(r'\b([a-zA-Z_]\w*)\s*\(')

    keywords = {
        "if", "for", "while", "switch", "catch", "sizeof", "typeof",
        "return", "else", "do", "case", "default", "break", "continue",
        "struct", "class", "enum", "union", "typedef", "define",
        "elif", "except", "with", "assert", "print",
        "function", "const", "let", "var", "new",
    }

    # Index des fonctions locales avec leurs plages
    local_functions = {}
    for sym in symbols:
        if sym.get("kind") in ("function", "method") and sym.get("line_start"):
            local_functions[sym["name"]] = {
                "start": sym["line_start"],
                "end": sym.get("line_end") or sym["line_start"] + 200,
            }

    in_block_comment = False

    for line_num, line in enumerate(lines, 1):
        stripped = line.strip()

        if "/*" in line:
            in_block_comment = True
        if "*/" in line:
            in_block_comment = False
            continue
        if in_block_comment:
            continue

        if stripped.startswith("//") or stripped.startswith("#"):
            continue

        for match in call_pattern.finditer(line):
            callee_name = match.group(1)

            if callee_name in keywords:
                continue

            if callee_name not in all_symbols and callee_name not in local_functions:
                continue

            # Trouver le caller
            caller_name = None
            for func_name, func_info in local_functions.items():
                if func_info["start"] <= line_num <= func_info["end"]:
                    caller_name = func_name
                    break

            if caller_name and caller_name != callee_name:
                calls.append({
                    "caller": caller_name,
                    "callee": callee_name,
                    "line": line_num,
                })

    return calls


# =============================================================================
# STEP 5: CALCULATE METRICS
# =============================================================================

def calculate_complexity(file_path: Path, language: str) -> dict[str, Any]:
    """Calcule la complexité cyclomatique d'un fichier."""
    complexity_sum = 0
    complexity_max = 0
    function_count = 0

    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")

        # Compter les mots-clés de branchement
        branch_keywords = [
            r'\bif\b', r'\belse\b', r'\belif\b', r'\bfor\b', r'\bwhile\b',
            r'\bcase\b', r'\bcatch\b', r'\b\?\s*:', r'\b&&\b', r'\b\|\|\b',
            r'\band\b', r'\bor\b',
        ]

        # Complexité par fonction (estimation)
        if language == "python":
            import ast
            try:
                with warnings.catch_warnings():
                    warnings.filterwarnings("ignore", category=SyntaxWarning)
                    tree = ast.parse(content)
                for node in ast.walk(tree):
                    if isinstance(node, ast.FunctionDef):
                        func_content = ast.unparse(node)
                        func_complexity = 1
                        for kw in branch_keywords:
                            func_complexity += len(re.findall(kw, func_content))
                        complexity_sum += func_complexity
                        complexity_max = max(complexity_max, func_complexity)
                        function_count += 1
            except Exception:
                pass
        else:
            # Estimation globale pour C/C++
            for kw in branch_keywords:
                complexity_sum += len(re.findall(kw, content))
            complexity_max = complexity_sum  # Approximation
            function_count = len(re.findall(r'\b\w+\s*\([^)]*\)\s*\{', content))

    except Exception:
        pass

    avg = round(complexity_sum / function_count, 2) if function_count > 0 else 0

    return {
        "sum": complexity_sum,
        "max": complexity_max,
        "avg": avg,
    }


def step_5_calculate_metrics(
    config: BootstrapConfig,
    logger: logging.Logger,
    stats: BootstrapStats,
    files: list[dict[str, Any]]
) -> None:
    """Étape 5 : Calculer les métriques."""
    print(f"\n{Colors.BOLD}Step 5/9:{Colors.RESET} Calculating metrics...")

    conn = sqlite3.connect(str(config.db_path))
    cursor = conn.cursor()

    progress = ProgressBar(len(files), "Metrics")

    for file_info in files:
        progress.update()

        file_id = file_info["id"]
        file_path = file_info["full_path"]
        language = file_info.get("language", "")

        complexity = calculate_complexity(file_path, language)

        # Score de documentation
        cursor.execute("""
            SELECT COUNT(*), SUM(CASE WHEN has_doc = 1 THEN 1 ELSE 0 END)
            FROM symbols WHERE file_id = ?
        """, (file_id,))
        row = cursor.fetchone()
        total_symbols = row[0] or 0
        documented = row[1] or 0
        doc_score = round((documented / total_symbols * 100) if total_symbols > 0 else 0)

        # Mettre à jour les métriques
        cursor.execute("""
            UPDATE files SET
                complexity_sum = ?,
                complexity_avg = ?,
                complexity_max = ?,
                documentation_score = ?
            WHERE id = ?
        """, (
            complexity["sum"],
            complexity["avg"],
            complexity["max"],
            doc_score,
            file_id,
        ))

        logger.debug(f"Metrics for {file_info['path']}: complexity={complexity['avg']}, doc={doc_score}%")

    progress.finish()
    conn.commit()
    conn.close()

    print(f"  {Colors.GREEN}✓{Colors.RESET} Calculated metrics for {len(files)} files")


# =============================================================================
# STEP 6: ANALYZE GIT ACTIVITY
# =============================================================================

def get_git_commits(file_path: str, days: int, project_root: Path) -> int:
    """Compte les commits pour un fichier dans les N derniers jours."""
    try:
        result = subprocess.run(
            ["git", "log", "--oneline", f"--since={days} days ago", "--", file_path],
            capture_output=True,
            text=True,
            cwd=str(project_root),
            timeout=10,
        )
        return len(result.stdout.strip().split("\n")) if result.stdout.strip() else 0
    except Exception:
        return 0


def get_git_contributors(file_path: str, project_root: Path) -> list[str]:
    """Récupère les contributeurs d'un fichier."""
    try:
        result = subprocess.run(
            ["git", "log", "--format=%an", "--", file_path],
            capture_output=True,
            text=True,
            cwd=str(project_root),
            timeout=10,
        )
        if result.stdout.strip():
            contributors = list(set(result.stdout.strip().split("\n")))
            return contributors[:10]  # Limite à 10
        return []
    except Exception:
        return []


def get_git_last_modified(file_path: str, project_root: Path) -> Optional[str]:
    """Récupère la date de dernière modification."""
    try:
        result = subprocess.run(
            ["git", "log", "-1", "--format=%aI", "--", file_path],
            capture_output=True,
            text=True,
            cwd=str(project_root),
            timeout=10,
        )
        return result.stdout.strip() if result.stdout.strip() else None
    except Exception:
        return None


def step_6_analyze_git(
    config: BootstrapConfig,
    logger: logging.Logger,
    stats: BootstrapStats,
    files: list[dict[str, Any]]
) -> None:
    """Étape 6 : Analyser l'activité Git."""
    print(f"\n{Colors.BOLD}Step 6/9:{Colors.RESET} Analyzing Git activity...")

    # Vérifier si on est dans un repo git
    if not (config.project_root / ".git").exists():
        print(f"  {Colors.YELLOW}⚠{Colors.RESET} Not a Git repository, skipping")
        stats.warnings.append("Not a Git repository")
        return

    conn = sqlite3.connect(str(config.db_path))
    cursor = conn.cursor()

    progress = ProgressBar(len(files), "Git analysis")

    for file_info in files:
        progress.update()

        file_id = file_info["id"]
        file_path = file_info["path"]

        commits_30d = get_git_commits(file_path, 30, config.project_root)
        commits_90d = get_git_commits(file_path, 90, config.project_root)
        commits_365d = get_git_commits(file_path, 365, config.project_root)
        contributors = get_git_contributors(file_path, config.project_root)
        last_modified = get_git_last_modified(file_path, config.project_root)

        cursor.execute("""
            UPDATE files SET
                commits_30d = ?,
                commits_90d = ?,
                commits_365d = ?,
                contributors_json = ?,
                last_modified = ?
            WHERE id = ?
        """, (
            commits_30d,
            commits_90d,
            commits_365d,
            json.dumps(contributors),
            last_modified,
            file_id,
        ))

        logger.debug(f"Git for {file_path}: {commits_30d}/{commits_90d}/{commits_365d} commits")

    progress.finish()
    conn.commit()
    conn.close()

    print(f"  {Colors.GREEN}✓{Colors.RESET} Analyzed Git history for {len(files)} files")


# =============================================================================
# STEP 7: MARK CRITICAL FILES
# =============================================================================

def matches_pattern(path: str, patterns: list[str]) -> bool:
    """Vérifie si un chemin correspond à un des patterns."""
    path_parts = path.split("/")

    for pattern in patterns:
        if "**" not in pattern:
            if fnmatch.fnmatch(path, pattern):
                return True
            continue

        # Pattern "**/suffix"
        if pattern.startswith("**/"):
            suffix = pattern[3:]
            for i in range(len(path_parts)):
                subpath = "/".join(path_parts[i:])
                if fnmatch.fnmatch(subpath, suffix):
                    return True
            continue

        # Pattern "prefix/**"
        if pattern.endswith("/**"):
            prefix = pattern[:-3]
            if path.startswith(prefix + "/") or path == prefix:
                return True
            continue

        # Pattern complexe
        if fnmatch.fnmatch(path, pattern.replace("**", "*")):
            return True

    return False


def check_security_content(file_path: Path) -> bool:
    """Vérifie si le fichier contient du code sensible."""
    security_patterns = [
        r'\bpassword\b', r'\bsecret\b', r'\btoken\b', r'\bapi_key\b',
        r'\bprivate_key\b', r'\bcredential\b',
        r'\bAES\b', r'\bRSA\b', r'\bSHA256\b', r'\bMD5\b', r'\bbcrypt\b',
        r'\bencrypt\b', r'\bdecrypt\b', r'\bhash\b',
    ]

    try:
        content = file_path.read_text(encoding="utf-8", errors="replace").lower()
        for pattern in security_patterns:
            if re.search(pattern, content, re.IGNORECASE):
                return True
    except Exception:
        pass

    return False


def step_7_mark_critical(
    config: BootstrapConfig,
    logger: logging.Logger,
    stats: BootstrapStats,
    files: list[dict[str, Any]]
) -> None:
    """Étape 7 : Marquer les fichiers critiques."""
    print(f"\n{Colors.BOLD}Step 7/9:{Colors.RESET} Marking critical files...")

    conn = sqlite3.connect(str(config.db_path))
    cursor = conn.cursor()

    critical_count = 0
    security_count = 0

    progress = ProgressBar(len(files), "Criticality")

    for file_info in files:
        progress.update()

        file_id = file_info["id"]
        file_path = file_info["path"]
        full_path = file_info["full_path"]

        is_critical = False
        criticality_reason = None
        security_sensitive = False

        # Vérifier les patterns critiques
        if matches_pattern(file_path, config.critical_paths):
            is_critical = True
            criticality_reason = "matches critical path pattern"
            critical_count += 1
        elif matches_pattern(file_path, config.high_importance_paths):
            is_critical = True
            criticality_reason = "matches high importance pattern"
            critical_count += 1

        # Vérifier le contenu sensible
        if check_security_content(full_path):
            security_sensitive = True
            security_count += 1
            if not is_critical:
                is_critical = True
                criticality_reason = "contains security-sensitive content"
                critical_count += 1

        if is_critical or security_sensitive:
            cursor.execute("""
                UPDATE files SET
                    is_critical = ?,
                    criticality_reason = ?,
                    security_sensitive = ?
                WHERE id = ?
            """, (
                1 if is_critical else 0,
                criticality_reason,
                1 if security_sensitive else 0,
                file_id,
            ))

            logger.debug(f"Marked {file_path} as critical={is_critical}, security={security_sensitive}")

    progress.finish()
    conn.commit()
    conn.close()

    print(f"  {Colors.GREEN}✓{Colors.RESET} Marked {critical_count} critical files, {security_count} security-sensitive")


# =============================================================================
# STEP 8: IMPORT DEFAULT PATTERNS
# =============================================================================

DEFAULT_PATTERNS = [
    # ==========================================================================
    # ERROR HANDLING PATTERNS
    # ==========================================================================
    {
        "name": "error_handling_malloc",
        "category": "error_handling",
        "title": "Check malloc return value",
        "description": "Always check if malloc/calloc/realloc returns NULL before using the pointer. Memory allocation can fail when system is low on memory.",
        "severity": "error",
        "good_example": "char *buf = malloc(size);\nif (buf == NULL) {\n    log_error(\"malloc failed\");\n    return -1;\n}",
        "bad_example": "char *buf = malloc(size);\nstrcpy(buf, data);  // Crash if malloc failed",
    },
    {
        "name": "error_handling_fopen",
        "category": "error_handling",
        "title": "Check fopen return value",
        "description": "Always check if fopen returns NULL before using the file handle. File may not exist or permissions may be denied.",
        "severity": "error",
        "good_example": "FILE *fp = fopen(path, \"r\");\nif (fp == NULL) {\n    perror(\"fopen failed\");\n    return -1;\n}",
        "bad_example": "FILE *fp = fopen(path, \"r\");\nfread(buf, 1, size, fp);  // Crash if fopen failed",
    },
    {
        "name": "error_handling_return_codes",
        "category": "error_handling",
        "title": "Check function return codes",
        "description": "Never ignore return codes from functions that can fail. Always check and handle errors appropriately.",
        "severity": "warning",
        "good_example": "int ret = process_data(data);\nif (ret != 0) {\n    log_error(\"process failed: %d\", ret);\n    return ret;\n}",
        "bad_example": "process_data(data);  // Return code ignored\nnext_step();",
    },
    {
        "name": "error_handling_errno",
        "category": "error_handling",
        "title": "Check errno after system calls",
        "description": "After system calls that set errno on failure, check errno to get detailed error information.",
        "severity": "info",
        "good_example": "if (write(fd, buf, len) < 0) {\n    log_error(\"write failed: %s\", strerror(errno));\n    return -1;\n}",
        "bad_example": "if (write(fd, buf, len) < 0) {\n    return -1;  // No error details\n}",
    },

    # ==========================================================================
    # MEMORY SAFETY PATTERNS (C/C++)
    # ==========================================================================
    {
        "name": "memory_safety_strncpy",
        "category": "memory_safety",
        "title": "Use strncpy instead of strcpy",
        "description": "Prefer strncpy over strcpy to prevent buffer overflows. Always null-terminate manually.",
        "severity": "warning",
        "good_example": "strncpy(dest, src, sizeof(dest) - 1);\ndest[sizeof(dest) - 1] = '\\0';",
        "bad_example": "strcpy(dest, src);  // Buffer overflow if src > dest size",
    },
    {
        "name": "memory_safety_snprintf",
        "category": "memory_safety",
        "title": "Use snprintf instead of sprintf",
        "description": "Prefer snprintf over sprintf to prevent buffer overflows. Check return value for truncation.",
        "severity": "warning",
        "good_example": "int ret = snprintf(buf, sizeof(buf), \"value: %d\", val);\nif (ret >= sizeof(buf)) {\n    log_warn(\"output truncated\");\n}",
        "bad_example": "sprintf(buf, \"value: %d\", val);  // Buffer overflow risk",
    },
    {
        "name": "memory_safety_free",
        "category": "memory_safety",
        "title": "Free allocated memory",
        "description": "Every malloc/calloc must have a corresponding free. Set pointer to NULL after free to prevent use-after-free.",
        "severity": "warning",
        "good_example": "char *buf = malloc(size);\nif (buf) {\n    // use buf\n    free(buf);\n    buf = NULL;\n}",
        "bad_example": "char *buf = malloc(size);\n// use buf\nreturn;  // Memory leak!",
    },
    {
        "name": "memory_safety_bounds_check",
        "category": "memory_safety",
        "title": "Check array bounds",
        "description": "Always verify array indices are within bounds before accessing. Prevents buffer overflows and underflows.",
        "severity": "error",
        "good_example": "if (index >= 0 && index < array_size) {\n    value = array[index];\n}",
        "bad_example": "value = array[index];  // No bounds check",
    },
    {
        "name": "memory_safety_null_deref",
        "category": "memory_safety",
        "title": "Check pointers before dereferencing",
        "description": "Always check that pointers are not NULL before dereferencing them.",
        "severity": "error",
        "good_example": "if (ptr != NULL) {\n    value = ptr->field;\n}",
        "bad_example": "value = ptr->field;  // Crash if ptr is NULL",
    },

    # ==========================================================================
    # NAMING CONVENTION PATTERNS
    # ==========================================================================
    {
        "name": "naming_functions",
        "category": "naming",
        "title": "Function naming convention",
        "description": "Functions should use snake_case and be prefixed with module name for clarity and to avoid naming conflicts.",
        "severity": "info",
        "good_example": "int lcd_init(void);\nint lcd_write(uint8_t *data);\nint gpio_set_pin(int pin, int value);",
        "bad_example": "int LCDInit(void);  // CamelCase\nint Write(uint8_t *data);  // No prefix",
    },
    {
        "name": "naming_constants",
        "category": "naming",
        "title": "Constant naming convention",
        "description": "Constants and macros should use UPPER_SNAKE_CASE to distinguish them from variables.",
        "severity": "info",
        "good_example": "#define MAX_BUFFER_SIZE 1024\nconst int DEFAULT_TIMEOUT = 30;",
        "bad_example": "#define maxBufferSize 1024  // Not uppercase",
    },
    {
        "name": "naming_types",
        "category": "naming",
        "title": "Type naming convention",
        "description": "User-defined types (structs, enums, typedefs) should use a consistent naming pattern, typically PascalCase or with _t suffix.",
        "severity": "info",
        "good_example": "typedef struct {\n    int x, y;\n} Point;\n\ntypedef enum { OK, ERROR } status_t;",
        "bad_example": "typedef struct { int x, y; } point;  // Inconsistent naming",
    },
    {
        "name": "naming_variables",
        "category": "naming",
        "title": "Variable naming convention",
        "description": "Variables should use snake_case and have descriptive names. Avoid single-letter names except for loop counters.",
        "severity": "info",
        "good_example": "int buffer_size = 1024;\nchar *file_path = \"/tmp/data.txt\";",
        "bad_example": "int bs = 1024;  // Unclear abbreviation\nchar *x;  // Non-descriptive",
    },

    # ==========================================================================
    # DOCUMENTATION PATTERNS
    # ==========================================================================
    {
        "name": "documentation_public",
        "category": "documentation",
        "title": "Document public functions",
        "description": "All public functions should have documentation comments describing purpose, parameters, return values, and possible errors.",
        "severity": "warning",
        "good_example": "/**\n * Initialize the LCD controller.\n * \n * @param config LCD configuration structure\n * @return 0 on success, -1 on error (sets errno)\n */\nint lcd_init(LCD_Config *config);",
        "bad_example": "int lcd_init(LCD_Config *config);  // No documentation",
    },
    {
        "name": "documentation_params",
        "category": "documentation",
        "title": "Document function parameters",
        "description": "Document all function parameters including valid ranges, ownership semantics (for pointers), and whether they can be NULL.",
        "severity": "info",
        "good_example": "/**\n * Write data to LCD.\n * @param data Buffer to write (must not be NULL)\n * @param len  Number of bytes (1-256)\n */\nint lcd_write(const uint8_t *data, size_t len);",
        "bad_example": "/** Write to LCD. */\nint lcd_write(const uint8_t *data, size_t len);",
    },
    {
        "name": "documentation_module",
        "category": "documentation",
        "title": "Document modules/files",
        "description": "Each source file should have a header comment describing its purpose and main responsibilities.",
        "severity": "info",
        "good_example": "/**\n * @file lcd_driver.c\n * @brief LCD display driver for HD44780 controllers\n * \n * Provides low-level interface for 16x2 character LCD.\n */",
        "bad_example": "// lcd_driver.c\n#include <stdio.h>  // No module documentation",
    },

    # ==========================================================================
    # SECURITY PATTERNS
    # ==========================================================================
    {
        "name": "security_input_validation",
        "category": "security",
        "title": "Validate external input",
        "description": "Always validate data from external sources (user input, network, files). Check length, format, and range.",
        "severity": "error",
        "good_example": "if (len > MAX_SIZE || len == 0) return -1;\nif (data == NULL) return -1;\nif (!is_valid_format(data)) return -1;",
        "bad_example": "memcpy(buf, user_data, user_len);  // No validation!",
    },
    {
        "name": "security_sql_injection",
        "category": "security",
        "title": "Prevent SQL injection",
        "description": "Use parameterized queries instead of string concatenation to prevent SQL injection attacks.",
        "severity": "error",
        "good_example": "cursor.execute(\"SELECT * FROM users WHERE id = ?\", (user_id,))",
        "bad_example": "cursor.execute(f\"SELECT * FROM users WHERE id = {user_id}\")  // SQL injection!",
    },
    {
        "name": "security_sensitive_data",
        "category": "security",
        "title": "Protect sensitive data",
        "description": "Never log or expose sensitive data (passwords, tokens, keys). Clear sensitive data from memory after use.",
        "severity": "error",
        "good_example": "// Process password\nverify_password(password);\nmemset(password, 0, sizeof(password));  // Clear from memory",
        "bad_example": "log_debug(\"User password: %s\", password);  // Never log passwords!",
    },

    # ==========================================================================
    # PERFORMANCE PATTERNS
    # ==========================================================================
    {
        "name": "performance_loop_invariant",
        "category": "performance",
        "title": "Move invariants out of loops",
        "description": "Move calculations that don't change inside the loop to outside. Reduces redundant computations.",
        "severity": "info",
        "good_example": "size_t len = strlen(str);\nfor (int i = 0; i < len; i++) {\n    process(str[i]);\n}",
        "bad_example": "for (int i = 0; i < strlen(str); i++) {  // strlen called each iteration\n    process(str[i]);\n}",
    },
    {
        "name": "performance_early_exit",
        "category": "performance",
        "title": "Use early exit conditions",
        "description": "Check for invalid conditions early and return/continue to avoid unnecessary processing.",
        "severity": "info",
        "good_example": "if (data == NULL || len == 0) return;\nif (already_processed) return;\n// Main processing here",
        "bad_example": "if (data != NULL && len > 0 && !already_processed) {\n    // Deeply nested processing\n}",
    },

    # ==========================================================================
    # PYTHON-SPECIFIC PATTERNS
    # ==========================================================================
    {
        "name": "python_exception_handling",
        "category": "error_handling",
        "title": "Handle exceptions properly",
        "description": "Catch specific exceptions, not bare except. Log or handle the error appropriately.",
        "severity": "warning",
        "good_example": "try:\n    result = process(data)\nexcept ValueError as e:\n    logger.error(f\"Invalid data: {e}\")\n    raise",
        "bad_example": "try:\n    result = process(data)\nexcept:  # Catches everything including KeyboardInterrupt!\n    pass",
    },
    {
        "name": "python_context_managers",
        "category": "memory_safety",
        "title": "Use context managers for resources",
        "description": "Use 'with' statements for files, locks, and other resources to ensure proper cleanup.",
        "severity": "warning",
        "good_example": "with open(path, 'r') as f:\n    data = f.read()\n# File automatically closed",
        "bad_example": "f = open(path, 'r')\ndata = f.read()\n# f.close() might never be called on exception",
    },
    {
        "name": "python_type_hints",
        "category": "documentation",
        "title": "Use type hints",
        "description": "Add type hints to function signatures for better documentation and IDE support.",
        "severity": "info",
        "good_example": "def process(data: list[str], count: int) -> dict[str, int]:\n    ...",
        "bad_example": "def process(data, count):\n    ...  # Types unclear",
    },
]


def step_8_import_patterns(
    config: BootstrapConfig,
    logger: logging.Logger,
    stats: BootstrapStats
) -> None:
    """Étape 8 : Importer les patterns par défaut."""
    print(f"\n{Colors.BOLD}Step 8/9:{Colors.RESET} Importing default patterns...")

    conn = sqlite3.connect(str(config.db_path))
    cursor = conn.cursor()

    imported = 0

    for pattern in DEFAULT_PATTERNS:
        try:
            cursor.execute("""
                INSERT OR IGNORE INTO patterns (
                    name, category, title, description, severity,
                    good_example, bad_example, scope, is_active
                ) VALUES (?, ?, ?, ?, ?, ?, ?, 'project', 1)
            """, (
                pattern["name"],
                pattern["category"],
                pattern["title"],
                pattern["description"],
                pattern["severity"],
                pattern.get("good_example", ""),
                pattern.get("bad_example", ""),
            ))

            if cursor.rowcount > 0:
                imported += 1
                logger.debug(f"Imported pattern: {pattern['name']}")
        except sqlite3.Error as e:
            logger.warning(f"Failed to import pattern {pattern['name']}: {e}")

    conn.commit()
    conn.close()

    print(f"  {Colors.GREEN}✓{Colors.RESET} Imported {imported} default patterns")


# =============================================================================
# STEP 9: VERIFY INTEGRITY
# =============================================================================

def step_9_verify_integrity(
    config: BootstrapConfig,
    logger: logging.Logger,
    stats: BootstrapStats
) -> bool:
    """Étape 9 : Vérifier l'intégrité de la base."""
    print(f"\n{Colors.BOLD}Step 9/9:{Colors.RESET} Verifying database integrity...")

    conn = sqlite3.connect(str(config.db_path))
    cursor = conn.cursor()

    issues = []

    # 1. Intégrité SQLite
    result = cursor.execute("PRAGMA integrity_check").fetchone()
    if result[0] != "ok":
        issues.append(f"SQLite integrity check failed: {result[0]}")

    # 2. Foreign key violations
    cursor.execute("PRAGMA foreign_key_check")
    fk_errors = cursor.fetchall()
    if fk_errors:
        issues.append(f"Foreign key violations: {len(fk_errors)}")

    # 3. Orphan symbols
    cursor.execute("""
        SELECT COUNT(*) FROM symbols s
        WHERE NOT EXISTS (SELECT 1 FROM files f WHERE f.id = s.file_id)
    """)
    orphan_symbols = cursor.fetchone()[0]
    if orphan_symbols > 0:
        issues.append(f"Orphan symbols: {orphan_symbols}")

    # 4. Stats
    cursor.execute("SELECT COUNT(*) FROM files")
    file_count = cursor.fetchone()[0]

    cursor.execute("SELECT COUNT(*) FROM symbols")
    symbol_count = cursor.fetchone()[0]

    cursor.execute("SELECT COUNT(*) FROM relations")
    relation_count = cursor.fetchone()[0]

    cursor.execute("SELECT COUNT(*) FROM file_relations")
    file_relation_count = cursor.fetchone()[0]

    cursor.execute("SELECT COUNT(*) FROM patterns")
    pattern_count = cursor.fetchone()[0]

    # 5. Test performance
    start = time.time()
    cursor.execute("""
        SELECT f.path, COUNT(s.id) as sym_count
        FROM files f
        LEFT JOIN symbols s ON s.file_id = f.id
        GROUP BY f.id
        LIMIT 100
    """)
    cursor.fetchall()
    query_time = (time.time() - start) * 1000

    if query_time > 100:
        stats.warnings.append(f"Query performance warning: {query_time:.0f}ms")

    conn.close()

    # Report
    if issues:
        for issue in issues:
            print(f"  {Colors.RED}✗{Colors.RESET} {issue}")
            stats.errors.append(issue)
        return False

    print(f"  {Colors.GREEN}✓{Colors.RESET} Database integrity verified")
    print(f"    Files: {file_count}")
    print(f"    Symbols: {symbol_count}")
    print(f"    Relations: {relation_count}")
    print(f"    File Relations: {file_relation_count}")
    print(f"    Patterns: {pattern_count}")
    print(f"    Query time: {query_time:.1f}ms")

    return True


# =============================================================================
# CHECKPOINT MANAGEMENT
# =============================================================================

@dataclass
class IndexCheckpoint:
    """Checkpoint d'indexation."""
    last_commit: str
    last_commit_short: str
    last_commit_message: str
    files_indexed: int
    symbols_indexed: int
    relations_indexed: int
    last_indexed_at: str
    duration_seconds: float
    index_mode: str
    schema_version: str = "2.0"


def get_current_commit(project_root: Path) -> tuple[str, str, str]:
    """
    Récupère les infos du commit HEAD actuel.

    Returns:
        Tuple (hash_complet, hash_court, message)
    """
    try:
        # Hash complet
        result = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            capture_output=True,
            text=True,
            cwd=str(project_root),
            timeout=10,
        )
        full_hash = result.stdout.strip()

        # Hash court
        result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            capture_output=True,
            text=True,
            cwd=str(project_root),
            timeout=10,
        )
        short_hash = result.stdout.strip()

        # Message du commit
        result = subprocess.run(
            ["git", "log", "-1", "--format=%s", "HEAD"],
            capture_output=True,
            text=True,
            cwd=str(project_root),
            timeout=10,
        )
        message = result.stdout.strip()[:100]  # Limiter à 100 caractères

        return full_hash, short_hash, message
    except Exception as e:
        return "", "", ""


def get_checkpoint(config: BootstrapConfig) -> Optional[IndexCheckpoint]:
    """
    Récupère le checkpoint d'indexation depuis la base.

    Returns:
        IndexCheckpoint ou None si pas de checkpoint
    """
    if not config.db_path.exists():
        return None

    try:
        conn = sqlite3.connect(str(config.db_path))
        conn.row_factory = sqlite3.Row
        cursor = conn.execute("SELECT * FROM index_checkpoints WHERE id = 1")
        row = cursor.fetchone()
        conn.close()

        if row:
            return IndexCheckpoint(
                last_commit=row["last_commit"],
                last_commit_short=row["last_commit_short"] or "",
                last_commit_message=row["last_commit_message"] or "",
                files_indexed=row["files_indexed"] or 0,
                symbols_indexed=row["symbols_indexed"] or 0,
                relations_indexed=row["relations_indexed"] or 0,
                last_indexed_at=row["last_indexed_at"],
                duration_seconds=row["duration_seconds"] or 0,
                index_mode=row["index_mode"] or "full",
                schema_version=row["schema_version"] or "2.0",
            )
        return None
    except sqlite3.Error:
        return None


def save_checkpoint(
    config: BootstrapConfig,
    stats: BootstrapStats,
    mode: str = "full"
) -> bool:
    """
    Enregistre le checkpoint d'indexation.

    Args:
        config: Configuration du bootstrap
        stats: Statistiques de l'indexation
        mode: Mode d'indexation ("full" ou "incremental")

    Returns:
        True si sauvegardé avec succès
    """
    full_hash, short_hash, message = get_current_commit(config.project_root)
    if not full_hash:
        return False

    try:
        conn = sqlite3.connect(str(config.db_path))

        # Utiliser INSERT OR REPLACE pour mettre à jour la ligne unique
        conn.execute("""
            INSERT OR REPLACE INTO index_checkpoints (
                id, last_commit, last_commit_short, last_commit_message,
                files_indexed, symbols_indexed, relations_indexed,
                last_indexed_at, duration_seconds, index_mode, schema_version
            ) VALUES (1, ?, ?, ?, ?, ?, ?, datetime('now'), ?, ?, '2.0')
        """, (
            full_hash,
            short_hash,
            message,
            stats.files_indexed,
            stats.symbols_indexed,
            stats.relations_indexed,
            stats.duration_seconds,
            mode,
        ))

        conn.commit()
        conn.close()
        return True
    except sqlite3.Error as e:
        print(f"  {Colors.RED}✗{Colors.RESET} Failed to save checkpoint: {e}")
        return False


def get_changed_files(
    config: BootstrapConfig,
    checkpoint: IndexCheckpoint
) -> dict[str, list[str]]:
    """
    Calcule les fichiers modifiés depuis le checkpoint.

    Args:
        config: Configuration du bootstrap
        checkpoint: Checkpoint de référence

    Returns:
        Dict avec les clés 'modified', 'added', 'deleted', 'renamed'
    """
    changes: dict[str, list[str]] = {
        "modified": [],
        "added": [],
        "deleted": [],
        "renamed": [],
    }

    try:
        # git diff --name-status pour avoir le type de changement
        result = subprocess.run(
            ["git", "diff", f"{checkpoint.last_commit}..HEAD", "--name-status"],
            capture_output=True,
            text=True,
            cwd=str(config.project_root),
            timeout=60,
        )

        if result.returncode != 0:
            return changes

        # Collecter toutes les extensions valides
        valid_extensions = set()
        for exts in config.extensions.values():
            valid_extensions.update(exts)

        for line in result.stdout.strip().split("\n"):
            if not line:
                continue

            parts = line.split("\t")
            if len(parts) < 2:
                continue

            status = parts[0]
            file_path = parts[1]

            # Vérifier l'extension
            ext = Path(file_path).suffix.lower()
            if ext not in valid_extensions:
                continue

            # Vérifier les exclusions
            if should_exclude(file_path, config.exclude_patterns):
                continue

            # Catégoriser le changement
            if status == "M":
                changes["modified"].append(file_path)
            elif status == "A":
                changes["added"].append(file_path)
            elif status == "D":
                changes["deleted"].append(file_path)
            elif status.startswith("R"):
                # Renommage : R100<tab>old_path<tab>new_path
                if len(parts) >= 3:
                    old_path = parts[1]
                    new_path = parts[2]
                    changes["deleted"].append(old_path)
                    changes["added"].append(new_path)
                    changes["renamed"].append(f"{old_path} -> {new_path}")

        return changes

    except subprocess.TimeoutExpired:
        print(f"  {Colors.YELLOW}⚠{Colors.RESET} Git diff timed out")
        return changes
    except Exception as e:
        print(f"  {Colors.YELLOW}⚠{Colors.RESET} Error getting changed files: {e}")
        return changes


# =============================================================================
# INCREMENTAL INDEXATION
# =============================================================================

def reindex_file(
    config: BootstrapConfig,
    logger: logging.Logger,
    file_path: str,
    conn: sqlite3.Connection
) -> tuple[int, int]:
    """
    Réindexe un seul fichier : supprime l'ancien index et réindexe.

    Args:
        config: Configuration du bootstrap
        logger: Logger
        file_path: Chemin du fichier (relatif)
        conn: Connexion SQLite

    Returns:
        Tuple (symbols_count, relations_count)
    """
    cursor = conn.cursor()
    full_path = config.project_root / file_path

    if not full_path.exists():
        logger.warning(f"File not found for reindexing: {file_path}")
        return 0, 0

    # 1. Supprimer les anciennes données
    cursor.execute("SELECT id FROM files WHERE path = ?", (file_path,))
    existing = cursor.fetchone()

    if existing:
        file_id = existing[0]
        # Supprimer les symboles (les relations sont supprimées en cascade)
        cursor.execute("DELETE FROM symbols WHERE file_id = ?", (file_id,))
        # Supprimer les relations de fichiers
        cursor.execute(
            "DELETE FROM file_relations WHERE source_file_id = ? OR target_file_id = ?",
            (file_id, file_id)
        )
        # Supprimer le fichier lui-même
        cursor.execute("DELETE FROM files WHERE id = ?", (file_id,))

    # 2. Réindexer le fichier
    # Détecter le langage
    ext = full_path.suffix.lower()
    language = None
    for lang, exts in config.extensions.items():
        if ext in exts:
            language = lang
            break

    if not language:
        logger.warning(f"Unknown language for {file_path}")
        return 0, 0

    # Lire le contenu
    try:
        content = full_path.read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        logger.warning(f"Cannot read {file_path}: {e}")
        return 0, 0

    # Calculer les métriques
    line_counts = count_lines(full_path)
    content_hash = get_content_hash(full_path)
    module = get_module_from_path(file_path, config.project_root)

    # Déterminer si c'est critique
    is_critical = matches_pattern(file_path, config.critical_paths) or \
                  matches_pattern(file_path, config.high_importance_paths)
    security_sensitive = check_security_content(full_path)

    # Insérer le fichier
    cursor.execute("""
        INSERT INTO files (
            path, filename, extension, module, language,
            lines_total, lines_code, lines_comment, lines_blank,
            content_hash, is_critical, security_sensitive, indexed_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'))
    """, (
        file_path, full_path.name, ext, module, language,
        line_counts["total"], line_counts["code"], line_counts["comment"], line_counts["blank"],
        content_hash, 1 if is_critical else 0, 1 if security_sensitive else 0,
    ))
    file_id = cursor.lastrowid

    # Parser les symboles
    symbols = []
    if language == "python":
        symbols = parse_python_file(full_path)
    elif language in ("c", "cpp") and shutil.which("ctags"):
        symbols = run_ctags(full_path)

    # Insérer les symboles
    symbols_count = 0
    for sym in symbols:
        cursor.execute("""
            INSERT INTO symbols (
                file_id, name, qualified_name, kind, line_start, line_end,
                signature, visibility, complexity, is_static,
                doc_comment, has_doc, base_classes_json
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            file_id,
            sym.get("name", ""),
            sym.get("qualified_name"),
            sym.get("kind", "unknown"),
            sym.get("line_start"),
            sym.get("line_end"),
            sym.get("signature", ""),
            sym.get("visibility", "public"),
            sym.get("complexity", 0),
            1 if sym.get("is_static") else 0,
            sym.get("doc_comment", ""),
            1 if sym.get("doc_comment") else 0,
            json.dumps(sym.get("base_classes")) if sym.get("base_classes") else None,
        ))
        symbols_count += 1

    logger.debug(f"Reindexed {file_path}: {symbols_count} symbols")
    return symbols_count, 0


def delete_file_from_index(
    config: BootstrapConfig,
    logger: logging.Logger,
    file_path: str,
    conn: sqlite3.Connection
) -> bool:
    """
    Supprime un fichier de l'index.

    Args:
        config: Configuration du bootstrap
        logger: Logger
        file_path: Chemin du fichier
        conn: Connexion SQLite

    Returns:
        True si supprimé
    """
    cursor = conn.cursor()

    cursor.execute("SELECT id FROM files WHERE path = ?", (file_path,))
    existing = cursor.fetchone()

    if existing:
        file_id = existing[0]
        # Les symboles et relations sont supprimés en cascade grâce aux FK
        cursor.execute("DELETE FROM files WHERE id = ?", (file_id,))
        logger.debug(f"Deleted from index: {file_path}")
        return True

    return False


def run_incremental(
    config: BootstrapConfig,
    logger: logging.Logger,
    checkpoint: IndexCheckpoint
) -> tuple[bool, BootstrapStats]:
    """
    Exécute l'indexation incrémentale.

    Args:
        config: Configuration du bootstrap
        logger: Logger
        checkpoint: Checkpoint de référence

    Returns:
        Tuple (success, stats)
    """
    stats = BootstrapStats()
    stats.start_time = time.time()

    print(f"\n{Colors.BOLD}{Colors.CYAN}Incremental Indexation{Colors.RESET}")
    print(f"{'=' * 60}")
    print(f"Checkpoint: {checkpoint.last_commit_short} ({checkpoint.last_indexed_at})")

    # Récupérer les fichiers modifiés
    changes = get_changed_files(config, checkpoint)

    total_changes = (
        len(changes["modified"]) +
        len(changes["added"]) +
        len(changes["deleted"])
    )

    if total_changes == 0:
        print(f"\n{Colors.GREEN}✓{Colors.RESET} Base already up to date")
        print(f"  Last indexed: {checkpoint.last_commit_short}")
        stats.end_time = time.time()
        return True, stats

    # Avertissement si beaucoup de fichiers
    if total_changes > 100:
        print(f"\n{Colors.YELLOW}⚠{Colors.RESET} {total_changes} files to reindex. This may take a while...")
        print(f"  (Consider using --full if you did a major rebase)")

    # Afficher le résumé des changements
    print(f"\n{Colors.BOLD}Changes detected:{Colors.RESET}")
    if changes["modified"]:
        print(f"  Modified: {len(changes['modified'])} files")
    if changes["added"]:
        print(f"  Added: {len(changes['added'])} files")
    if changes["deleted"]:
        print(f"  Deleted: {len(changes['deleted'])} files")
    if changes["renamed"]:
        print(f"  Renamed: {len(changes['renamed'])} files")

    # Ouvrir la connexion
    conn = sqlite3.connect(str(config.db_path))
    conn.execute("PRAGMA foreign_keys = ON")

    try:
        # Supprimer les fichiers supprimés
        for file_path in changes["deleted"]:
            delete_file_from_index(config, logger, file_path, conn)

        # Réindexer les fichiers modifiés et ajoutés
        files_to_reindex = changes["modified"] + changes["added"]
        total_symbols = 0
        total_relations = 0

        progress = ProgressBar(len(files_to_reindex), "Reindexing")
        for file_path in files_to_reindex:
            progress.update()
            syms, rels = reindex_file(config, logger, file_path, conn)
            total_symbols += syms
            total_relations += rels
            stats.files_indexed += 1

        progress.finish()

        conn.commit()

        stats.symbols_indexed = total_symbols
        stats.relations_indexed = total_relations
        stats.end_time = time.time()

        print(f"\n{Colors.GREEN}✓{Colors.RESET} Incremental indexation complete")
        print(f"  Files reindexed: {stats.files_indexed}")
        print(f"  Symbols indexed: {stats.symbols_indexed}")
        print(f"  Duration: {stats.duration_seconds:.1f}s")

        return True, stats

    except Exception as e:
        conn.rollback()
        stats.errors.append(str(e))
        logger.error(f"Incremental indexation failed: {e}")
        print(f"\n{Colors.RED}✗{Colors.RESET} Incremental indexation failed: {e}")
        return False, stats
    finally:
        conn.close()


def show_status(config: BootstrapConfig) -> int:
    """
    Affiche l'état actuel de l'indexation.

    Args:
        config: Configuration du bootstrap

    Returns:
        Code de retour (0 = OK)
    """
    print(f"\n{Colors.BOLD}{Colors.CYAN}AgentDB Index Status{Colors.RESET}")
    print(f"{'=' * 60}")
    print(f"Project: {config.project_root}")
    print(f"Database: {config.db_path}")

    # Vérifier si la base existe
    if not config.db_path.exists():
        print(f"\n{Colors.YELLOW}⚠{Colors.RESET} Database not found")
        print(f"  Run: python bootstrap.py --full")
        return 1

    # Récupérer le checkpoint
    checkpoint = get_checkpoint(config)

    if not checkpoint:
        print(f"\n{Colors.YELLOW}⚠{Colors.RESET} No checkpoint found")
        print(f"  The database exists but hasn't been indexed yet.")
        print(f"  Run: python bootstrap.py --full")
        return 1

    # Afficher les infos du checkpoint
    print(f"\n{Colors.BOLD}Last Indexation:{Colors.RESET}")
    print(f"  Commit: {checkpoint.last_commit_short} ({checkpoint.last_commit[:12]}...)")
    print(f"  Message: {checkpoint.last_commit_message[:60]}...")
    print(f"  Date: {checkpoint.last_indexed_at}")
    print(f"  Mode: {checkpoint.index_mode}")
    print(f"  Duration: {checkpoint.duration_seconds:.1f}s")

    print(f"\n{Colors.BOLD}Statistics:{Colors.RESET}")
    print(f"  Files indexed: {checkpoint.files_indexed}")
    print(f"  Symbols indexed: {checkpoint.symbols_indexed}")
    print(f"  Relations indexed: {checkpoint.relations_indexed}")

    # Calculer les changements depuis le checkpoint
    changes = get_changed_files(config, checkpoint)
    total_changes = (
        len(changes["modified"]) +
        len(changes["added"]) +
        len(changes["deleted"])
    )

    if total_changes == 0:
        print(f"\n{Colors.GREEN}✓{Colors.RESET} Index is up to date")
    else:
        print(f"\n{Colors.YELLOW}!{Colors.RESET} {total_changes} files changed since last indexation:")
        if changes["modified"]:
            print(f"    Modified: {len(changes['modified'])}")
            for f in changes["modified"][:5]:
                print(f"      - {f}")
            if len(changes["modified"]) > 5:
                print(f"      ... and {len(changes['modified']) - 5} more")
        if changes["added"]:
            print(f"    Added: {len(changes['added'])}")
            for f in changes["added"][:5]:
                print(f"      + {f}")
            if len(changes["added"]) > 5:
                print(f"      ... and {len(changes['added']) - 5} more")
        if changes["deleted"]:
            print(f"    Deleted: {len(changes['deleted'])}")
            for f in changes["deleted"][:5]:
                print(f"      - {f}")
            if len(changes["deleted"]) > 5:
                print(f"      ... and {len(changes['deleted']) - 5} more")

        print(f"\n  Run: python bootstrap.py --incremental")

    return 0


# =============================================================================
# MAIN
# =============================================================================

def print_report(stats: BootstrapStats, success: bool) -> None:
    """Affiche le rapport final."""
    print(f"\n{'=' * 60}")

    if success:
        print(f"{Colors.GREEN}{Colors.BOLD}BOOTSTRAP COMPLETED SUCCESSFULLY{Colors.RESET}")
    else:
        print(f"{Colors.RED}{Colors.BOLD}BOOTSTRAP COMPLETED WITH ERRORS{Colors.RESET}")

    print(f"{'=' * 60}")
    print(f"\n{Colors.BOLD}Summary:{Colors.RESET}")
    print(f"  Files scanned:    {stats.files_scanned}")
    print(f"  Files indexed:    {stats.files_indexed}")
    print(f"  Files skipped:    {stats.files_skipped}")
    print(f"  Symbols indexed:  {stats.symbols_indexed}")
    print(f"  Relations:        {stats.relations_indexed + stats.file_relations_indexed}")
    print(f"  Duration:         {stats.duration_seconds:.1f}s")

    if stats.warnings:
        print(f"\n{Colors.YELLOW}Warnings ({len(stats.warnings)}):{Colors.RESET}")
        for w in stats.warnings[:5]:
            print(f"  - {w}")
        if len(stats.warnings) > 5:
            print(f"  ... and {len(stats.warnings) - 5} more")

    if stats.errors:
        print(f"\n{Colors.RED}Errors ({len(stats.errors)}):{Colors.RESET}")
        for e in stats.errors[:5]:
            print(f"  - {e}")
        if len(stats.errors) > 5:
            print(f"  ... and {len(stats.errors) - 5} more")

    print()


def main() -> int:
    """Point d'entrée principal."""
    parser = argparse.ArgumentParser(
        description="AgentDB Bootstrap - Initialize the agent database"
    )
    parser.add_argument(
        "--project-root",
        type=Path,
        default=Path.cwd(),
        help="Root directory of the project (default: current directory)"
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Enable verbose output"
    )

    # Mode d'indexation (mutuellement exclusif)
    mode_group = parser.add_mutually_exclusive_group()
    mode_group.add_argument(
        "--full",
        action="store_true",
        help="Full indexation (default on first run)"
    )
    mode_group.add_argument(
        "--incremental",
        action="store_true",
        help="Incremental indexation (only changed files since last checkpoint)"
    )
    mode_group.add_argument(
        "--status",
        action="store_true",
        help="Show current indexation status"
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Force full indexation even if checkpoint exists"
    )

    args = parser.parse_args()

    # Configuration
    config = BootstrapConfig.from_project_root(args.project_root.resolve())

    # Setup logging
    logger = setup_logging(config.logs_dir)

    # =========================================================================
    # MODE: --status
    # =========================================================================
    if args.status:
        return show_status(config)

    # =========================================================================
    # MODE: --incremental
    # =========================================================================
    if args.incremental:
        # Vérifier qu'un checkpoint existe
        checkpoint = get_checkpoint(config)

        if not checkpoint:
            print(f"\n{Colors.RED}✗{Colors.RESET} No checkpoint found.")
            print(f"  Run first: python bootstrap.py --full")
            return 1

        # Lancer l'indexation incrémentale
        success, stats = run_incremental(config, logger, checkpoint)

        # Sauvegarder le checkpoint si succès
        if success:
            save_checkpoint(config, stats, mode="incremental")

        logger.info(f"Incremental indexation {'completed' if success else 'failed'} in {stats.duration_seconds:.1f}s")
        return 0 if success else 1

    # =========================================================================
    # MODE: --full (ou par défaut)
    # =========================================================================
    stats = BootstrapStats()
    stats.start_time = time.time()

    # Vérifier si un checkpoint existe et suggérer --incremental
    if not args.full and not args.force:
        checkpoint = get_checkpoint(config)
        if checkpoint:
            print(f"\n{Colors.YELLOW}!{Colors.RESET} A checkpoint already exists: {checkpoint.last_commit_short}")
            print(f"  Use --incremental to update only changed files")
            print(f"  Use --full to force a complete re-indexation")
            print(f"  Use --status to see current status")
            return 1

    # Banner
    print(f"\n{Colors.BOLD}{Colors.CYAN}AgentDB Bootstrap (Full Mode){Colors.RESET}")
    print(f"{'=' * 60}")
    print(f"Project root: {config.project_root}")
    print(f"Database: {config.db_path}")

    success = True

    try:
        # Step 1: Create structure
        if not step_1_create_structure(config, logger):
            success = False

        # Step 2: Initialize schema
        if success and not step_2_init_schema(config, logger):
            success = False

        # Step 3: Scan files
        files = []
        if success:
            files = step_3_scan_files(config, logger, stats)

        # Step 4: Index symbols
        if success and files:
            step_4_index_symbols(config, logger, stats, files)

        # Step 4b: Extract relations (calls)
        if success and files:
            step_4b_extract_relations(config, logger, stats, files)

        # Step 5: Calculate metrics
        if success and files:
            step_5_calculate_metrics(config, logger, stats, files)

        # Step 6: Analyze Git
        if success and files:
            step_6_analyze_git(config, logger, stats, files)

        # Step 7: Mark critical files
        if success and files:
            step_7_mark_critical(config, logger, stats, files)

        # Step 8: Import patterns
        if success:
            step_8_import_patterns(config, logger, stats)

        # Step 9: Verify integrity
        if success:
            if not step_9_verify_integrity(config, logger, stats):
                success = False

    except KeyboardInterrupt:
        print(f"\n\n{Colors.YELLOW}Bootstrap interrupted by user{Colors.RESET}")
        success = False
    except Exception as e:
        logger.exception("Bootstrap failed")
        print(f"\n{Colors.RED}Bootstrap failed: {e}{Colors.RESET}")
        stats.errors.append(str(e))
        success = False

    stats.end_time = time.time()

    # Final report
    print_report(stats, success)

    # Update meta and save checkpoint
    if success:
        try:
            conn = sqlite3.connect(str(config.db_path))
            conn.execute(
                "UPDATE agentdb_meta SET value = ? WHERE key = 'project_name'",
                (config.project_root.name,)
            )
            conn.execute(
                "INSERT OR REPLACE INTO agentdb_meta (key, value) VALUES ('last_bootstrap', datetime('now'))"
            )
            conn.commit()
            conn.close()

            # Sauvegarder le checkpoint
            save_checkpoint(config, stats, mode="full")
            print(f"\n{Colors.GREEN}✓{Colors.RESET} Checkpoint saved")

        except Exception as e:
            logger.warning(f"Failed to update meta/checkpoint: {e}")

    logger.info(f"Bootstrap {'completed' if success else 'failed'} in {stats.duration_seconds:.1f}s")

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
