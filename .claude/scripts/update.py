#!/usr/bin/env python3
"""
AgentDB Update - Mise à jour incrémentale.

Ce script met à jour AgentDB après des modifications de fichiers.
Il est conçu pour être appelé :
- Manuellement après des changements
- Automatiquement via un hook post-commit
- Par le pipeline CI

Durée cible : < 5 secondes pour un commit typique.

Usage:
    python -m scripts.update [--commit HASH]
    python .claude/scripts/update.py [--commit HASH]
"""

from __future__ import annotations

import argparse
import hashlib
import json
import logging
import re
import shutil
import sqlite3
import subprocess
import sys
import time
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any, Optional

# =============================================================================
# CONFIGURATION
# =============================================================================

class Colors:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    CYAN = "\033[96m"


if not sys.stdout.isatty():
    for attr in dir(Colors):
        if not attr.startswith("_"):
            setattr(Colors, attr, "")


@dataclass
class UpdateStats:
    """Statistiques de mise à jour."""
    files_updated: int = 0
    files_added: int = 0
    files_removed: int = 0
    symbols_added: int = 0
    symbols_removed: int = 0
    errors: list[str] = field(default_factory=list)
    start_time: float = 0
    end_time: float = 0

    @property
    def duration_ms(self) -> float:
        return (self.end_time - self.start_time) * 1000


# =============================================================================
# LOGGER
# =============================================================================

def setup_logging(verbose: bool = False) -> logging.Logger:
    """Configure le logging."""
    logger = logging.getLogger("agentdb.update")
    logger.setLevel(logging.DEBUG if verbose else logging.INFO)

    if not logger.handlers:
        ch = logging.StreamHandler()
        ch.setLevel(logging.DEBUG if verbose else logging.INFO)
        ch.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
        logger.addHandler(ch)

    return logger


logger = setup_logging()


# =============================================================================
# FUNCTION 1: GET_MODIFIED_FILES
# =============================================================================

def get_modified_files(
    project_root: Path,
    commit: Optional[str] = None,
    include_staged: bool = True
) -> dict[str, list[str]]:
    """
    Utilise git diff pour trouver les fichiers modifiés.

    Args:
        project_root: Racine du projet
        commit: Commit de référence (défaut: HEAD~1)
        include_staged: Inclure les fichiers stagés

    Returns:
        Dict avec 'modified', 'added', 'deleted'
    """
    result = {
        "modified": [],
        "added": [],
        "deleted": [],
    }

    # Vérifier qu'on est dans un repo git
    if not (project_root / ".git").exists():
        logger.warning("Not a Git repository")
        return result

    ref = commit or "HEAD~1"

    try:
        # Obtenir les fichiers modifiés avec leur status
        cmd = ["git", "diff", "--name-status", ref]
        proc = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            cwd=str(project_root),
            timeout=30,
        )

        if proc.returncode != 0:
            # Peut échouer si c'est le premier commit
            logger.debug(f"git diff failed: {proc.stderr}")
            return result

        for line in proc.stdout.strip().split("\n"):
            if not line:
                continue

            parts = line.split("\t", 1)
            if len(parts) != 2:
                continue

            status, file_path = parts

            # Filtrer par extensions indexables
            if not _is_indexable_file(file_path):
                continue

            if status.startswith("A"):
                result["added"].append(file_path)
            elif status.startswith("D"):
                result["deleted"].append(file_path)
            elif status.startswith("M") or status.startswith("R"):
                result["modified"].append(file_path)

        # Ajouter les fichiers stagés si demandé
        if include_staged:
            staged_cmd = ["git", "diff", "--name-status", "--cached"]
            staged_proc = subprocess.run(
                staged_cmd,
                capture_output=True,
                text=True,
                cwd=str(project_root),
                timeout=30,
            )

            if staged_proc.returncode == 0:
                for line in staged_proc.stdout.strip().split("\n"):
                    if not line:
                        continue
                    parts = line.split("\t", 1)
                    if len(parts) != 2:
                        continue

                    status, file_path = parts
                    if not _is_indexable_file(file_path):
                        continue

                    if status.startswith("A") and file_path not in result["added"]:
                        result["added"].append(file_path)
                    elif status.startswith("D") and file_path not in result["deleted"]:
                        result["deleted"].append(file_path)
                    elif status.startswith("M") and file_path not in result["modified"]:
                        result["modified"].append(file_path)

    except subprocess.TimeoutExpired:
        logger.error("git diff timed out")
    except Exception as e:
        logger.error(f"Error getting modified files: {e}")

    return result


def _is_indexable_file(file_path: str) -> bool:
    """Vérifie si un fichier doit être indexé."""
    indexable_extensions = {
        ".c", ".h", ".cpp", ".hpp", ".cc", ".hh", ".cxx", ".hxx",
        ".py", ".pyi",
        ".js", ".jsx", ".mjs", ".ts", ".tsx",
        ".rs", ".go",
    }

    ext = Path(file_path).suffix.lower()
    return ext in indexable_extensions


# =============================================================================
# FUNCTION 2: UPDATE_FILE
# =============================================================================

def update_file(
    db_path: Path,
    project_root: Path,
    file_path: str,
    stats: UpdateStats
) -> bool:
    """
    Met à jour un fichier dans la base de données.

    1. Supprime les anciens symboles et relations
    2. Réindexe le fichier
    3. Met à jour les métriques

    Args:
        db_path: Chemin vers la base de données
        project_root: Racine du projet
        file_path: Chemin relatif du fichier
        stats: Statistiques à mettre à jour

    Returns:
        True si succès
    """
    full_path = project_root / file_path

    if not full_path.exists():
        logger.warning(f"File not found: {file_path}")
        return False

    conn = sqlite3.connect(str(db_path))
    conn.row_factory = sqlite3.Row
    cursor = conn.cursor()

    try:
        # 1. Trouver le fichier existant
        cursor.execute("SELECT id FROM files WHERE path = ?", (file_path,))
        row = cursor.fetchone()

        if not row:
            # Nouveau fichier - l'ajouter
            return _add_new_file(cursor, conn, project_root, file_path, stats)

        file_id = row["id"]

        # 2. Supprimer les anciens symboles (CASCADE supprime les relations)
        cursor.execute("SELECT COUNT(*) FROM symbols WHERE file_id = ?", (file_id,))
        old_symbol_count = cursor.fetchone()[0]
        stats.symbols_removed += old_symbol_count

        cursor.execute("DELETE FROM symbols WHERE file_id = ?", (file_id,))

        # 3. Supprimer les relations de fichiers sortantes
        cursor.execute(
            "DELETE FROM file_relations WHERE source_file_id = ?",
            (file_id,)
        )

        # 4. Réindexer le fichier
        new_symbols = _parse_file(full_path)

        for sym in new_symbols:
            cursor.execute("""
                INSERT INTO symbols (
                    file_id, name, kind, line_start, line_end,
                    signature, doc_comment, has_doc
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                file_id,
                sym.get("name", ""),
                sym.get("kind", "unknown"),
                sym.get("line_start"),
                sym.get("line_end"),
                sym.get("signature", ""),
                sym.get("doc_comment", ""),
                1 if sym.get("doc_comment") else 0,
            ))
            stats.symbols_added += 1

        # 5. Réextraire les includes/imports
        language = _get_language(full_path.suffix)
        includes = _extract_includes(full_path, language)

        for inc in includes:
            cursor.execute(
                "SELECT id FROM files WHERE path LIKE ?",
                (f"%{inc['target']}%",)
            )
            target = cursor.fetchone()
            if target:
                cursor.execute("""
                    INSERT OR IGNORE INTO file_relations (
                        source_file_id, target_file_id, relation_type, line_number
                    ) VALUES (?, ?, 'includes', ?)
                """, (file_id, target[0], inc["line"]))

        # 6. Mettre à jour les métriques du fichier
        line_counts = _count_lines(full_path)
        complexity = _calculate_complexity(full_path, language)
        content_hash = _get_content_hash(full_path)

        # Score de documentation
        cursor.execute("""
            SELECT COUNT(*), SUM(CASE WHEN has_doc = 1 THEN 1 ELSE 0 END)
            FROM symbols WHERE file_id = ?
        """, (file_id,))
        row = cursor.fetchone()
        total_symbols = row[0] or 0
        documented = row[1] or 0
        doc_score = round((documented / total_symbols * 100) if total_symbols > 0 else 0)

        cursor.execute("""
            UPDATE files SET
                lines_total = ?,
                lines_code = ?,
                lines_comment = ?,
                lines_blank = ?,
                complexity_sum = ?,
                complexity_avg = ?,
                complexity_max = ?,
                documentation_score = ?,
                content_hash = ?,
                indexed_at = datetime('now')
            WHERE id = ?
        """, (
            line_counts["total"],
            line_counts["code"],
            line_counts["comment"],
            line_counts["blank"],
            complexity["sum"],
            complexity["avg"],
            complexity["max"],
            doc_score,
            content_hash,
            file_id,
        ))

        conn.commit()
        stats.files_updated += 1
        logger.debug(f"Updated: {file_path} ({len(new_symbols)} symbols)")
        return True

    except Exception as e:
        conn.rollback()
        logger.error(f"Error updating {file_path}: {e}")
        stats.errors.append(f"{file_path}: {e}")
        return False

    finally:
        conn.close()


def _add_new_file(
    cursor: sqlite3.Cursor,
    conn: sqlite3.Connection,
    project_root: Path,
    file_path: str,
    stats: UpdateStats
) -> bool:
    """Ajoute un nouveau fichier à l'index."""
    full_path = project_root / file_path

    try:
        language = _get_language(full_path.suffix)
        line_counts = _count_lines(full_path)
        content_hash = _get_content_hash(full_path)
        module = _get_module_from_path(file_path)

        cursor.execute("""
            INSERT INTO files (
                path, filename, extension, module, language,
                lines_total, lines_code, lines_comment, lines_blank,
                content_hash, indexed_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'))
        """, (
            file_path,
            full_path.name,
            full_path.suffix,
            module,
            language,
            line_counts["total"],
            line_counts["code"],
            line_counts["comment"],
            line_counts["blank"],
            content_hash,
        ))

        file_id = cursor.lastrowid

        # Parser et indexer les symboles
        symbols = _parse_file(full_path)
        for sym in symbols:
            cursor.execute("""
                INSERT INTO symbols (
                    file_id, name, kind, line_start, line_end,
                    signature, doc_comment, has_doc
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                file_id,
                sym.get("name", ""),
                sym.get("kind", "unknown"),
                sym.get("line_start"),
                sym.get("line_end"),
                sym.get("signature", ""),
                sym.get("doc_comment", ""),
                1 if sym.get("doc_comment") else 0,
            ))
            stats.symbols_added += 1

        conn.commit()
        stats.files_added += 1
        logger.debug(f"Added: {file_path}")
        return True

    except Exception as e:
        conn.rollback()
        logger.error(f"Error adding {file_path}: {e}")
        stats.errors.append(f"{file_path}: {e}")
        return False


def remove_file(
    db_path: Path,
    file_path: str,
    stats: UpdateStats
) -> bool:
    """
    Supprime un fichier de l'index.

    Args:
        db_path: Chemin vers la base de données
        file_path: Chemin relatif du fichier
        stats: Statistiques à mettre à jour

    Returns:
        True si succès
    """
    conn = sqlite3.connect(str(db_path))
    cursor = conn.cursor()

    try:
        # Compter les symboles avant suppression
        cursor.execute("""
            SELECT COUNT(*) FROM symbols s
            JOIN files f ON s.file_id = f.id
            WHERE f.path = ?
        """, (file_path,))
        symbol_count = cursor.fetchone()[0]
        stats.symbols_removed += symbol_count

        # Supprimer (CASCADE supprime symboles et relations)
        cursor.execute("DELETE FROM files WHERE path = ?", (file_path,))

        if cursor.rowcount > 0:
            conn.commit()
            stats.files_removed += 1
            logger.debug(f"Removed: {file_path}")
            return True
        else:
            logger.debug(f"File not in index: {file_path}")
            return False

    except Exception as e:
        conn.rollback()
        logger.error(f"Error removing {file_path}: {e}")
        stats.errors.append(f"{file_path}: {e}")
        return False

    finally:
        conn.close()


# =============================================================================
# FUNCTION 3: UPDATE_ACTIVITY
# =============================================================================

def update_activity(
    db_path: Path,
    project_root: Path,
    file_paths: Optional[list[str]] = None
) -> int:
    """
    Met à jour les métriques d'activité Git.

    Args:
        db_path: Chemin vers la base de données
        project_root: Racine du projet
        file_paths: Fichiers à mettre à jour (défaut: tous)

    Returns:
        Nombre de fichiers mis à jour
    """
    conn = sqlite3.connect(str(db_path))
    conn.row_factory = sqlite3.Row
    cursor = conn.cursor()

    updated = 0

    try:
        # Sélectionner les fichiers à mettre à jour
        if file_paths:
            placeholders = ",".join("?" * len(file_paths))
            cursor.execute(
                f"SELECT id, path FROM files WHERE path IN ({placeholders})",
                file_paths
            )
        else:
            cursor.execute("SELECT id, path FROM files")

        files = cursor.fetchall()

        for file_row in files:
            file_id = file_row["id"]
            file_path = file_row["path"]

            # Compter les commits par période
            commits_30d = _get_git_commits(file_path, 30, project_root)
            commits_90d = _get_git_commits(file_path, 90, project_root)
            commits_365d = _get_git_commits(file_path, 365, project_root)
            last_modified = _get_git_last_modified(file_path, project_root)
            contributors = _get_git_contributors(file_path, project_root)

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

            updated += 1

        conn.commit()
        logger.debug(f"Updated activity for {updated} files")
        return updated

    except Exception as e:
        conn.rollback()
        logger.error(f"Error updating activity: {e}")
        return 0

    finally:
        conn.close()


def _get_git_commits(file_path: str, days: int, project_root: Path) -> int:
    """Compte les commits pour un fichier."""
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


def _get_git_last_modified(file_path: str, project_root: Path) -> Optional[str]:
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


def _get_git_contributors(file_path: str, project_root: Path) -> list[str]:
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
            return list(set(result.stdout.strip().split("\n")))[:10]
        return []
    except Exception:
        return []


# =============================================================================
# FUNCTION 4: UPDATE_INCREMENTAL
# =============================================================================

def update_incremental(
    project_root: Path,
    commit: Optional[str] = None,
    update_git_activity: bool = True,
    verbose: bool = False
) -> dict[str, Any]:
    """
    Orchestre la mise à jour incrémentale complète.

    Étapes:
    1. Identifier les fichiers modifiés via git diff
    2. Pour chaque fichier modifié: supprimer anciens symboles, réindexer
    3. Mettre à jour les métriques d'activité Git

    Durée cible: < 5 secondes pour un commit typique.

    Args:
        project_root: Racine du projet
        commit: Commit de référence (défaut: HEAD~1)
        update_git_activity: Mettre à jour les stats git
        verbose: Mode verbeux

    Returns:
        Rapport de mise à jour
    """
    if verbose:
        setup_logging(verbose=True)

    stats = UpdateStats()
    stats.start_time = time.time()

    db_path = project_root / ".claude" / "agentdb" / "db.sqlite"

    if not db_path.exists():
        logger.error(f"Database not found: {db_path}")
        logger.error("Run bootstrap first: python -m scripts.bootstrap")
        return {
            "success": False,
            "error": "Database not found",
        }

    print(f"{Colors.BOLD}AgentDB Incremental Update{Colors.RESET}")
    print(f"{'=' * 40}")

    # 1. Identifier les fichiers modifiés
    print(f"\n{Colors.CYAN}Detecting changes...{Colors.RESET}")
    changes = get_modified_files(project_root, commit)

    total_changes = len(changes["modified"]) + len(changes["added"]) + len(changes["deleted"])

    if total_changes == 0:
        print(f"  {Colors.GREEN}✓{Colors.RESET} No changes detected")
        stats.end_time = time.time()
        return {
            "success": True,
            "no_changes": True,
            "duration_ms": stats.duration_ms,
        }

    print(f"  Modified: {len(changes['modified'])}")
    print(f"  Added: {len(changes['added'])}")
    print(f"  Deleted: {len(changes['deleted'])}")

    # 2. Traiter les fichiers supprimés
    if changes["deleted"]:
        print(f"\n{Colors.CYAN}Removing deleted files...{Colors.RESET}")
        for file_path in changes["deleted"]:
            remove_file(db_path, file_path, stats)

    # 3. Traiter les fichiers ajoutés
    if changes["added"]:
        print(f"\n{Colors.CYAN}Adding new files...{Colors.RESET}")
        for file_path in changes["added"]:
            update_file(db_path, project_root, file_path, stats)

    # 4. Traiter les fichiers modifiés
    if changes["modified"]:
        print(f"\n{Colors.CYAN}Updating modified files...{Colors.RESET}")
        for file_path in changes["modified"]:
            update_file(db_path, project_root, file_path, stats)

    # 5. Mettre à jour l'activité Git pour les fichiers touchés
    if update_git_activity:
        print(f"\n{Colors.CYAN}Updating Git activity...{Colors.RESET}")
        all_touched = changes["modified"] + changes["added"]
        if all_touched:
            updated_count = update_activity(db_path, project_root, all_touched)
            print(f"  {Colors.GREEN}✓{Colors.RESET} Updated activity for {updated_count} files")

    stats.end_time = time.time()

    # Rapport final
    print(f"\n{'=' * 40}")
    if stats.errors:
        print(f"{Colors.YELLOW}Update completed with errors{Colors.RESET}")
    else:
        print(f"{Colors.GREEN}Update completed successfully{Colors.RESET}")

    print(f"\n{Colors.BOLD}Summary:{Colors.RESET}")
    print(f"  Files updated:  {stats.files_updated}")
    print(f"  Files added:    {stats.files_added}")
    print(f"  Files removed:  {stats.files_removed}")
    print(f"  Symbols added:  {stats.symbols_added}")
    print(f"  Symbols removed: {stats.symbols_removed}")
    print(f"  Duration:       {stats.duration_ms:.0f}ms")

    if stats.errors:
        print(f"\n{Colors.RED}Errors:{Colors.RESET}")
        for err in stats.errors[:5]:
            print(f"  - {err}")

    print()

    return {
        "success": len(stats.errors) == 0,
        "files_updated": stats.files_updated,
        "files_added": stats.files_added,
        "files_removed": stats.files_removed,
        "symbols_added": stats.symbols_added,
        "symbols_removed": stats.symbols_removed,
        "duration_ms": stats.duration_ms,
        "errors": stats.errors,
    }


# =============================================================================
# HELPER FUNCTIONS
# =============================================================================

def _get_language(ext: str) -> Optional[str]:
    """Détermine le langage à partir de l'extension."""
    lang_map = {
        ".c": "c", ".h": "c",
        ".cpp": "cpp", ".hpp": "cpp", ".cc": "cpp", ".hh": "cpp",
        ".cxx": "cpp", ".hxx": "cpp",
        ".py": "python", ".pyi": "python",
        ".js": "javascript", ".jsx": "javascript", ".mjs": "javascript",
        ".ts": "typescript", ".tsx": "typescript",
        ".rs": "rust",
        ".go": "go",
    }
    return lang_map.get(ext.lower())


def _get_module_from_path(path: str) -> str:
    """Détermine le module à partir du chemin."""
    parts = Path(path).parts
    if len(parts) >= 2:
        if parts[0] in ("src", "lib", "app", "pkg"):
            return parts[1] if len(parts) > 1 else parts[0]
        return parts[0]
    return "root"


def _count_lines(file_path: Path) -> dict[str, int]:
    """Compte les lignes d'un fichier."""
    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
        lines = content.split("\n")

        total = len(lines)
        blank = sum(1 for line in lines if not line.strip())

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

        return {
            "total": total,
            "code": max(0, total - blank - comment),
            "comment": comment,
            "blank": blank,
        }
    except Exception:
        return {"total": 0, "code": 0, "comment": 0, "blank": 0}


def _get_content_hash(file_path: Path) -> str:
    """Calcule le hash MD5 du contenu."""
    try:
        return hashlib.md5(file_path.read_bytes()).hexdigest()
    except Exception:
        return ""


def _calculate_complexity(file_path: Path, language: str) -> dict[str, Any]:
    """Calcule la complexité cyclomatique."""
    complexity_sum = 0
    complexity_max = 0
    function_count = 0

    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")

        branch_keywords = [
            r'\bif\b', r'\belse\b', r'\belif\b', r'\bfor\b', r'\bwhile\b',
            r'\bcase\b', r'\bcatch\b', r'\b&&\b', r'\b\|\|\b',
        ]

        if language == "python":
            import ast
            try:
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
            for kw in branch_keywords:
                complexity_sum += len(re.findall(kw, content))
            complexity_max = complexity_sum
            function_count = len(re.findall(r'\b\w+\s*\([^)]*\)\s*\{', content))

    except Exception:
        pass

    return {
        "sum": complexity_sum,
        "max": complexity_max,
        "avg": round(complexity_sum / function_count, 2) if function_count > 0 else 0,
    }


def _parse_file(file_path: Path) -> list[dict[str, Any]]:
    """Parse un fichier et retourne les symboles."""
    language = _get_language(file_path.suffix)

    if language == "python":
        return _parse_python_file(file_path)
    elif language in ("c", "cpp"):
        return _parse_with_ctags(file_path)
    else:
        return []


def _parse_python_file(file_path: Path) -> list[dict[str, Any]]:
    """Parse un fichier Python avec ast."""
    import ast

    try:
        content = file_path.read_text(encoding="utf-8")
        tree = ast.parse(content)
        symbols = []

        for node in ast.walk(tree):
            if isinstance(node, ast.FunctionDef):
                args = []
                for arg in node.args.args:
                    arg_str = arg.arg
                    if arg.annotation:
                        arg_str += f": {ast.unparse(arg.annotation)}"
                    args.append(arg_str)

                signature = f"({', '.join(args)})"
                if node.returns:
                    signature += f" -> {ast.unparse(node.returns)}"

                symbols.append({
                    "name": node.name,
                    "kind": "function",
                    "line_start": node.lineno,
                    "line_end": node.end_lineno,
                    "signature": signature,
                    "doc_comment": ast.get_docstring(node),
                })

            elif isinstance(node, ast.ClassDef):
                symbols.append({
                    "name": node.name,
                    "kind": "class",
                    "line_start": node.lineno,
                    "line_end": node.end_lineno,
                    "doc_comment": ast.get_docstring(node),
                })

        return symbols
    except Exception:
        return []


def _parse_with_ctags(file_path: Path) -> list[dict[str, Any]]:
    """Parse avec ctags."""
    if not shutil.which("ctags"):
        return []

    try:
        result = subprocess.run(
            ["ctags", "--output-format=json", "--fields=*", "-o", "-", str(file_path)],
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
                    })
                except json.JSONDecodeError:
                    pass

        return symbols
    except Exception:
        return []


def _extract_includes(file_path: Path, language: str) -> list[dict[str, Any]]:
    """Extrait les includes/imports."""
    includes = []

    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
        lines = content.split("\n")

        for i, line in enumerate(lines, 1):
            line = line.strip()

            if language in ("c", "cpp"):
                match = re.match(r'#include\s*[<"]([^>"]+)[>"]', line)
                if match:
                    includes.append({"target": match.group(1), "line": i})

            elif language == "python":
                match = re.match(r'(?:from\s+(\S+)\s+)?import\s+(.+)', line)
                if match:
                    module = match.group(1) or match.group(2).split(",")[0].strip()
                    includes.append({"target": module.split()[0], "line": i})

    except Exception:
        pass

    return includes


# =============================================================================
# CLI
# =============================================================================

def main() -> int:
    """Point d'entrée CLI."""
    parser = argparse.ArgumentParser(
        description="AgentDB Incremental Update - Update database after changes"
    )
    parser.add_argument(
        "--commit", "-c",
        help="Git commit reference (default: HEAD~1)"
    )
    parser.add_argument(
        "--no-activity",
        action="store_true",
        help="Skip Git activity update"
    )
    parser.add_argument(
        "--project-root",
        type=Path,
        default=Path.cwd(),
        help="Project root directory"
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Verbose output"
    )

    args = parser.parse_args()

    result = update_incremental(
        project_root=args.project_root.resolve(),
        commit=args.commit,
        update_git_activity=not args.no_activity,
        verbose=args.verbose,
    )

    return 0 if result.get("success") else 1


if __name__ == "__main__":
    sys.exit(main())
