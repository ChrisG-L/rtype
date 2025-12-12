#!/usr/bin/env python3
"""
Import Bug History - Peuple la table error_history depuis l'historique Git.

Ce script parcourt l'historique Git pour identifier les commits de correction
de bugs et les importer dans la table error_history d'AgentDB.

Usage:
    python .claude/scripts/import-bug-history.py [options]

Options:
    --dry-run       Affiche ce qui serait importé sans modifier la base
    --since DAYS    Limite à une période (défaut: 365 jours)
    --verbose       Affiche les détails de chaque commit trouvé
"""

from __future__ import annotations

import argparse
import re
import sqlite3
import subprocess
import sys
from dataclasses import dataclass
from datetime import datetime, timedelta
from pathlib import Path
from typing import Optional


# =============================================================================
# CONFIGURATION
# =============================================================================

class Colors:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    CYAN = "\033[96m"


if not sys.stdout.isatty():
    for attr in dir(Colors):
        if not attr.startswith("_"):
            setattr(Colors, attr, "")


# Mots-clés pour identifier les commits de correction
BUGFIX_KEYWORDS = [
    r'\bfix\b', r'\bfixed\b', r'\bfixes\b', r'\bfixing\b',
    r'\bbug\b', r'\bbugfix\b',
    r'\bhotfix\b',
    r'\bpatch\b', r'\bpatched\b',
    r'\bresolve[ds]?\b',
    r'\bissue\b',
    r'\bcrash\b', r'\bcrashed\b', r'\bcrashes\b',
    r'\berror\b',
    r'\brepair\b', r'\brepaired\b',
    r'\bcorrect\b', r'\bcorrected\b', r'\bcorrection\b',
]

# Mots-clés pour déterminer le type d'erreur
ERROR_TYPE_PATTERNS = {
    'security': [
        r'\bsecurity\b', r'\bvuln\b', r'\bvulnerability\b',
        r'\bxss\b', r'\bsql.?injection\b', r'\binjection\b',
        r'\bauth\b', r'\bauthentication\b', r'\bauthorization\b',
        r'\bcve\b', r'\bcwe\b',
    ],
    'crash': [
        r'\bcrash\b', r'\bsegfault\b', r'\bsegmentation\b',
        r'\bpanic\b', r'\babort\b', r'\bfreeze\b', r'\bhang\b',
        r'\bnull.?pointer\b', r'\bnpe\b',
    ],
    'memory': [
        r'\bmemory\b', r'\bleak\b', r'\bmemleak\b',
        r'\bbuffer.?overflow\b', r'\boverflow\b', r'\bunderflow\b',
        r'\buse.?after.?free\b', r'\bdouble.?free\b',
        r'\bout.?of.?bounds\b', r'\boob\b',
    ],
    'regression': [
        r'\bregression\b', r'\bregress\b',
        r'\brevert\b', r'\breverted\b',
        r'\brollback\b',
    ],
    'performance': [
        r'\bperformance\b', r'\bperf\b',
        r'\bslow\b', r'\btimeout\b',
        r'\boptimiz\b',
    ],
    'logic': [
        r'\blogic\b', r'\bwrong\b', r'\bincorrect\b',
        r'\bcalculation\b', r'\bcompute\b',
    ],
}

# Mots-clés pour déterminer la sévérité
SEVERITY_PATTERNS = {
    'critical': [
        r'\bcritical\b', r'\bsecurity\b', r'\bvuln\b',
        r'\bcrash\b', r'\bpanic\b', r'\bdata.?loss\b',
        r'\bproduction\b', r'\bprod\b', r'\burgent\b',
    ],
    'high': [
        r'\bhigh\b', r'\bsevere\b', r'\bimportant\b',
        r'\bregression\b', r'\bmajor\b',
    ],
    'medium': [
        r'\bmedium\b', r'\bmoderate\b',
    ],
    'low': [
        r'\blow\b', r'\bminor\b', r'\btrivial\b', r'\btypo\b',
    ],
}

# Extensions de fichiers de code à considérer
CODE_EXTENSIONS = {
    '.c', '.cpp', '.h', '.hpp', '.cc', '.hh', '.cxx', '.hxx',
    '.py', '.pyi',
    '.js', '.jsx', '.ts', '.tsx',
    '.go', '.rs', '.java',
    '.sh', '.bash',
}


@dataclass
class BugfixCommit:
    """Représente un commit de correction de bug."""
    commit_hash: str
    commit_short: str
    date: str
    author: str
    message: str
    files: list[str]
    error_type: str
    severity: str


@dataclass
class ImportStats:
    """Statistiques d'importation."""
    commits_scanned: int = 0
    bugfix_commits_found: int = 0
    entries_created: int = 0
    entries_skipped: int = 0
    errors: list[str] = None

    def __post_init__(self):
        if self.errors is None:
            self.errors = []


# =============================================================================
# FONCTIONS UTILITAIRES
# =============================================================================

def run_git_command(args: list[str], cwd: Optional[Path] = None) -> str:
    """Exécute une commande git et retourne la sortie."""
    try:
        result = subprocess.run(
            ['git'] + args,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=60,
        )
        if result.returncode != 0:
            return ""
        return result.stdout.strip()
    except subprocess.TimeoutExpired:
        return ""
    except Exception:
        return ""


def is_bugfix_commit(message: str) -> bool:
    """Détermine si un message de commit indique une correction de bug."""
    message_lower = message.lower()
    for pattern in BUGFIX_KEYWORDS:
        if re.search(pattern, message_lower):
            return True
    return False


def determine_error_type(message: str) -> str:
    """Détermine le type d'erreur basé sur le message du commit."""
    message_lower = message.lower()

    for error_type, patterns in ERROR_TYPE_PATTERNS.items():
        for pattern in patterns:
            if re.search(pattern, message_lower):
                return error_type

    return 'bug'  # Type par défaut


def determine_severity(message: str, error_type: str) -> str:
    """Détermine la sévérité basée sur le message et le type d'erreur."""
    message_lower = message.lower()

    # D'abord, chercher les mots-clés explicites
    for severity, patterns in SEVERITY_PATTERNS.items():
        for pattern in patterns:
            if re.search(pattern, message_lower):
                return severity

    # Sinon, déduire de l'error_type
    if error_type in ('security', 'crash', 'memory'):
        return 'high'
    if error_type == 'regression':
        return 'high'
    if error_type == 'performance':
        return 'medium'

    return 'medium'  # Sévérité par défaut


def clean_commit_message(message: str) -> str:
    """Nettoie un message de commit pour l'affichage."""
    # Prendre seulement la première ligne
    first_line = message.split('\n')[0]
    # Limiter la longueur
    if len(first_line) > 200:
        first_line = first_line[:197] + '...'
    return first_line


def is_code_file(filepath: str) -> bool:
    """Vérifie si un fichier est un fichier de code."""
    path = Path(filepath)
    return path.suffix.lower() in CODE_EXTENSIONS


# =============================================================================
# FONCTION PRINCIPALE
# =============================================================================

def find_bugfix_commits(
    project_root: Path,
    since_days: int,
    verbose: bool = False,
) -> list[BugfixCommit]:
    """
    Parcourt l'historique Git et retourne les commits de correction de bugs.
    """
    bugfixes = []

    # Calculer la date de début
    since_date = (datetime.now() - timedelta(days=since_days)).strftime('%Y-%m-%d')

    # Récupérer les commits depuis la date
    log_output = run_git_command([
        'log',
        f'--since={since_date}',
        '--format=%H|%h|%aI|%an|%s',
        '--no-merges',
    ], cwd=project_root)

    if not log_output:
        return bugfixes

    for line in log_output.split('\n'):
        if not line.strip():
            continue

        parts = line.split('|', 4)
        if len(parts) < 5:
            continue

        commit_hash, commit_short, date, author, message = parts

        # Vérifier si c'est un bugfix
        if not is_bugfix_commit(message):
            continue

        # Récupérer les fichiers modifiés
        files_output = run_git_command([
            'diff-tree',
            '--no-commit-id',
            '--name-only',
            '-r',
            commit_hash,
        ], cwd=project_root)

        if not files_output:
            continue

        # Filtrer les fichiers de code
        files = [f for f in files_output.split('\n') if f and is_code_file(f)]

        if not files:
            continue

        # Déterminer le type et la sévérité
        error_type = determine_error_type(message)
        severity = determine_severity(message, error_type)

        bugfix = BugfixCommit(
            commit_hash=commit_hash,
            commit_short=commit_short,
            date=date[:10],  # YYYY-MM-DD
            author=author,
            message=clean_commit_message(message),
            files=files,
            error_type=error_type,
            severity=severity,
        )

        bugfixes.append(bugfix)

        if verbose:
            print(f"  {Colors.GREEN}✓{Colors.RESET} {commit_short}: {bugfix.message[:60]}...")
            print(f"    Type: {error_type}, Sévérité: {severity}, Fichiers: {len(files)}")

    return bugfixes


def import_to_database(
    db_path: Path,
    bugfixes: list[BugfixCommit],
    dry_run: bool = False,
) -> ImportStats:
    """
    Importe les commits bugfix dans la table error_history.
    """
    stats = ImportStats()

    if dry_run:
        # En dry-run, on simule juste
        for bugfix in bugfixes:
            stats.bugfix_commits_found += 1
            stats.entries_created += len(bugfix.files)
        return stats

    if not db_path.exists():
        stats.errors.append(f"Database not found: {db_path}")
        return stats

    conn = sqlite3.connect(str(db_path))
    cursor = conn.cursor()

    try:
        for bugfix in bugfixes:
            stats.bugfix_commits_found += 1

            for filepath in bugfix.files:
                # Vérifier si l'entrée existe déjà
                cursor.execute("""
                    SELECT id FROM error_history
                    WHERE file_path = ? AND fix_commit = ?
                """, (filepath, bugfix.commit_hash))

                if cursor.fetchone():
                    stats.entries_skipped += 1
                    continue

                # Insérer la nouvelle entrée
                cursor.execute("""
                    INSERT INTO error_history (
                        file_path,
                        error_type,
                        severity,
                        title,
                        description,
                        discovered_at,
                        resolved_at,
                        discovered_by,
                        fix_commit,
                        is_regression
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """, (
                    filepath,
                    bugfix.error_type,
                    bugfix.severity,
                    bugfix.message,
                    f"Bug corrigé dans le commit {bugfix.commit_short}",
                    bugfix.date,
                    bugfix.date,
                    bugfix.author,
                    bugfix.commit_hash,
                    1 if bugfix.error_type == 'regression' else 0,
                ))

                stats.entries_created += 1

        conn.commit()

    except sqlite3.Error as e:
        stats.errors.append(f"Database error: {e}")
        conn.rollback()
    finally:
        conn.close()

    return stats


def print_summary(stats: ImportStats, dry_run: bool = False):
    """Affiche un résumé de l'importation."""
    mode = f"{Colors.YELLOW}[DRY-RUN]{Colors.RESET} " if dry_run else ""

    print(f"\n{Colors.BOLD}═══════════════════════════════════════════════════════════════{Colors.RESET}")
    print(f"{Colors.BOLD}  {mode}RÉSUMÉ DE L'IMPORTATION{Colors.RESET}")
    print(f"{Colors.BOLD}═══════════════════════════════════════════════════════════════{Colors.RESET}")
    print(f"  Commits bugfix trouvés : {Colors.CYAN}{stats.bugfix_commits_found}{Colors.RESET}")
    print(f"  Entrées créées         : {Colors.GREEN}{stats.entries_created}{Colors.RESET}")
    if stats.entries_skipped > 0:
        print(f"  Entrées ignorées       : {Colors.YELLOW}{stats.entries_skipped}{Colors.RESET} (déjà présentes)")
    if stats.errors:
        print(f"  Erreurs                : {Colors.RED}{len(stats.errors)}{Colors.RESET}")
        for error in stats.errors:
            print(f"    {Colors.RED}✗{Colors.RESET} {error}")
    print(f"{Colors.BOLD}═══════════════════════════════════════════════════════════════{Colors.RESET}\n")


def main():
    parser = argparse.ArgumentParser(
        description="Import bug history from Git commits into AgentDB",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help="Affiche ce qui serait importé sans modifier la base",
    )
    parser.add_argument(
        '--since',
        type=int,
        default=365,
        metavar='DAYS',
        help="Limite à une période en jours (défaut: 365)",
    )
    parser.add_argument(
        '--verbose',
        action='store_true',
        help="Affiche les détails de chaque commit trouvé",
    )
    parser.add_argument(
        '--project-root',
        type=Path,
        default=None,
        help="Racine du projet (défaut: répertoire courant)",
    )

    args = parser.parse_args()

    # Déterminer les chemins
    if args.project_root:
        project_root = args.project_root.resolve()
    else:
        # Essayer de trouver la racine du projet
        cwd = Path.cwd()
        if (cwd / '.claude').exists():
            project_root = cwd
        elif (cwd / '.git').exists():
            project_root = cwd
        else:
            # Remonter pour trouver .claude ou .git
            for parent in cwd.parents:
                if (parent / '.claude').exists():
                    project_root = parent
                    break
            else:
                project_root = cwd

    db_path = project_root / '.claude' / 'agentdb' / 'db.sqlite'

    # Afficher le contexte
    print(f"\n{Colors.BOLD}Import Bug History{Colors.RESET}")
    print(f"  Projet  : {project_root}")
    print(f"  Base    : {db_path}")
    print(f"  Période : {args.since} jours")
    if args.dry_run:
        print(f"  Mode    : {Colors.YELLOW}DRY-RUN{Colors.RESET}")
    print()

    # Vérifier que la base existe (sauf en dry-run)
    if not args.dry_run and not db_path.exists():
        print(f"{Colors.RED}Erreur:{Colors.RESET} Base de données non trouvée: {db_path}")
        print("Exécutez d'abord: python .claude/scripts/bootstrap.py")
        sys.exit(1)

    # Trouver les commits bugfix
    print(f"{Colors.CYAN}Analyse de l'historique Git...{Colors.RESET}")
    bugfixes = find_bugfix_commits(project_root, args.since, args.verbose)

    if not bugfixes:
        print(f"\n{Colors.YELLOW}Aucun commit de correction trouvé.{Colors.RESET}")
        print("Vérifiez que vos messages de commit contiennent des mots-clés comme:")
        print("  fix, bug, hotfix, patch, resolve, crash, error, etc.")
        sys.exit(0)

    # Importer dans la base
    print(f"\n{Colors.CYAN}{'Simulation de l' if args.dry_run else 'L'}importation...{Colors.RESET}")
    stats = import_to_database(db_path, bugfixes, args.dry_run)

    # Afficher le résumé
    print_summary(stats, args.dry_run)

    if stats.errors:
        sys.exit(1)

    if args.dry_run and stats.entries_created > 0:
        print(f"Pour effectuer l'importation réelle, relancez sans --dry-run")


if __name__ == '__main__':
    main()
