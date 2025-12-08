#!/usr/bin/env python3
"""
AgentDB Maintenance - Tâches de maintenance de la base.

Ce script effectue les tâches de maintenance périodiques :
- VACUUM : Récupérer l'espace des lignes supprimées
- ANALYZE : Mettre à jour les statistiques pour l'optimiseur
- INTEGRITY : Vérifier l'intégrité référentielle
- CLEANUP : Supprimer les données obsolètes (vieux snapshots, etc.)
- BACKUP : Sauvegarder la base
- STATS : Afficher les statistiques

Usage:
    # Toutes les tâches de maintenance
    python -m scripts.maintenance --all

    # Tâches spécifiques
    python -m scripts.maintenance --vacuum --analyze
    python -m scripts.maintenance --cleanup --days 30
    python -m scripts.maintenance --backup /path/to/backup.sqlite
    python -m scripts.maintenance --stats
"""

from __future__ import annotations

import argparse
import logging
import shutil
import sys
from datetime import datetime
from pathlib import Path
from typing import Any, Optional

sys.path.insert(0, str(Path(__file__).parent.parent))

from agentdb import Database

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger("maintenance")


# =============================================================================
# MAINTENANCE TASKS
# =============================================================================

class MaintenanceRunner:
    """
    Exécute les tâches de maintenance.
    """

    def __init__(self, db_path: Path) -> None:
        """
        Initialise le runner.

        Args:
            db_path: Chemin vers la base SQLite
        """
        self.db_path = db_path
        self.db: Optional[Database] = None

    def connect(self) -> None:
        """Connecte à la base."""
        # TODO: Implémenter
        pass

    def close(self) -> None:
        """Ferme la connexion."""
        # TODO: Implémenter
        pass

    # -------------------------------------------------------------------------
    # VACUUM
    # -------------------------------------------------------------------------

    def vacuum(self) -> dict[str, Any]:
        """
        Exécute VACUUM pour récupérer l'espace.

        Returns:
            Rapport avec taille avant/après
        """
        logger.info("Running VACUUM...")

        # TODO: Implémenter
        # 1. Récupérer la taille avant
        # 2. Exécuter VACUUM
        # 3. Récupérer la taille après
        result = {
            "size_before": 0,
            "size_after": 0,
            "space_freed": 0
        }
        return result

    # -------------------------------------------------------------------------
    # ANALYZE
    # -------------------------------------------------------------------------

    def analyze(self) -> dict[str, Any]:
        """
        Exécute ANALYZE pour optimiser les requêtes.

        Returns:
            Rapport
        """
        logger.info("Running ANALYZE...")

        # TODO: Implémenter
        # sqlite3 db.sqlite "ANALYZE;"
        result = {
            "tables_analyzed": 0
        }
        return result

    # -------------------------------------------------------------------------
    # INTEGRITY CHECK
    # -------------------------------------------------------------------------

    def check_integrity(self) -> dict[str, Any]:
        """
        Vérifie l'intégrité de la base.

        Returns:
            Rapport avec les problèmes trouvés
        """
        logger.info("Checking database integrity...")

        # TODO: Implémenter
        # 1. PRAGMA integrity_check
        # 2. Vérifier les FK orphelines
        # 3. Vérifier la cohérence des données
        result = {
            "sqlite_integrity": "ok",
            "foreign_keys_ok": True,
            "orphaned_symbols": 0,
            "orphaned_relations": 0,
            "issues": []
        }
        return result

    # -------------------------------------------------------------------------
    # CLEANUP
    # -------------------------------------------------------------------------

    def cleanup(self, days: int = 30) -> dict[str, Any]:
        """
        Supprime les données obsolètes.

        Args:
            days: Supprimer les données plus vieilles que N jours

        Returns:
            Rapport avec compteurs de suppression
        """
        logger.info(f"Cleaning up data older than {days} days...")

        # TODO: Implémenter
        # 1. Supprimer les vieux snapshots
        # 2. Archiver/supprimer les vieux pipeline_runs
        result = {
            "snapshots_deleted": 0,
            "old_runs_archived": 0
        }
        return result

    def cleanup_snapshots(self, days: int = 30) -> int:
        """
        Supprime les snapshots plus vieux que N jours.

        Args:
            days: Seuil en jours

        Returns:
            Nombre de snapshots supprimés
        """
        # TODO: Implémenter
        # DELETE FROM snapshot_symbols WHERE created_at < date('now', '-N days')
        pass

    def archive_old_runs(self, keep_last: int = 100) -> int:
        """
        Archive les vieux runs (garde les N derniers détaillés).

        Args:
            keep_last: Nombre de runs à garder en détail

        Returns:
            Nombre de runs archivés
        """
        # TODO: Implémenter
        # Garder les métriques mais supprimer issues_json, files_json
        pass

    # -------------------------------------------------------------------------
    # BACKUP
    # -------------------------------------------------------------------------

    def backup(self, backup_path: Path) -> dict[str, Any]:
        """
        Sauvegarde la base de données.

        Args:
            backup_path: Chemin de destination

        Returns:
            Rapport
        """
        logger.info(f"Backing up to {backup_path}...")

        # TODO: Implémenter
        # Option 1: Copie simple du fichier
        # Option 2: sqlite3 .backup
        result = {
            "backup_path": str(backup_path),
            "size_bytes": 0,
            "success": False
        }
        return result

    def restore(self, backup_path: Path) -> dict[str, Any]:
        """
        Restaure la base depuis une sauvegarde.

        Args:
            backup_path: Chemin de la sauvegarde

        Returns:
            Rapport
        """
        logger.info(f"Restoring from {backup_path}...")

        # TODO: Implémenter
        # 1. Vérifier que la sauvegarde existe
        # 2. Fermer les connexions
        # 3. Copier le fichier
        result = {
            "restored_from": str(backup_path),
            "success": False
        }
        return result

    # -------------------------------------------------------------------------
    # STATISTICS
    # -------------------------------------------------------------------------

    def get_stats(self) -> dict[str, Any]:
        """
        Récupère les statistiques de la base.

        Returns:
            Dict avec toutes les stats
        """
        logger.info("Gathering database statistics...")

        # TODO: Implémenter
        # Compter chaque table
        # Taille du fichier
        # Dernière mise à jour
        stats = {
            "database": {
                "path": str(self.db_path),
                "size_bytes": 0,
                "size_human": "0 B",
                "schema_version": "unknown"
            },
            "tables": {
                "files": 0,
                "symbols": 0,
                "relations": 0,
                "file_relations": 0,
                "error_history": 0,
                "pipeline_runs": 0,
                "patterns": 0,
                "architecture_decisions": 0,
                "critical_paths": 0,
                "snapshot_symbols": 0
            },
            "metrics": {
                "total_lines_of_code": 0,
                "avg_complexity": 0,
                "critical_files": 0,
                "files_with_tests": 0
            },
            "activity": {
                "last_indexed_at": None,
                "last_run_at": None,
                "runs_last_30d": 0
            }
        }
        return stats

    def print_stats(self, stats: dict[str, Any]) -> None:
        """
        Affiche les statistiques de manière lisible.

        Args:
            stats: Dict de statistiques
        """
        print("\n" + "=" * 60)
        print("AgentDB Statistics")
        print("=" * 60)

        print("\nDatabase:")
        print(f"  Path: {stats['database']['path']}")
        print(f"  Size: {stats['database']['size_human']}")
        print(f"  Schema version: {stats['database']['schema_version']}")

        print("\nTables:")
        for table, count in stats['tables'].items():
            print(f"  {table}: {count:,}")

        print("\nMetrics:")
        for key, value in stats['metrics'].items():
            print(f"  {key}: {value:,}" if isinstance(value, int) else f"  {key}: {value}")

        print("\nActivity:")
        for key, value in stats['activity'].items():
            print(f"  {key}: {value}")

        print("=" * 60 + "\n")

    # -------------------------------------------------------------------------
    # RUN ALL
    # -------------------------------------------------------------------------

    def run_all(self, cleanup_days: int = 30) -> dict[str, Any]:
        """
        Exécute toutes les tâches de maintenance.

        Args:
            cleanup_days: Seuil pour le cleanup

        Returns:
            Rapport complet
        """
        logger.info("Running all maintenance tasks...")

        self.connect()

        try:
            results = {
                "vacuum": self.vacuum(),
                "analyze": self.analyze(),
                "integrity": self.check_integrity(),
                "cleanup": self.cleanup(cleanup_days),
                "stats": self.get_stats()
            }

            # Vérifier s'il y a des problèmes
            has_issues = bool(results["integrity"].get("issues"))

            results["overall"] = {
                "success": not has_issues,
                "timestamp": datetime.now().isoformat()
            }

            return results

        finally:
            self.close()


# =============================================================================
# HELPERS
# =============================================================================

def human_readable_size(size_bytes: int) -> str:
    """Convertit des bytes en taille lisible."""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size_bytes < 1024:
            return f"{size_bytes:.1f} {unit}"
        size_bytes /= 1024
    return f"{size_bytes:.1f} TB"


# =============================================================================
# CLI
# =============================================================================

def main() -> int:
    """Point d'entrée CLI."""
    parser = argparse.ArgumentParser(
        description="AgentDB maintenance tasks"
    )

    # Tâches
    parser.add_argument("--all", "-a", action="store_true",
                        help="Run all maintenance tasks")
    parser.add_argument("--vacuum", action="store_true",
                        help="Run VACUUM to reclaim space")
    parser.add_argument("--analyze", action="store_true",
                        help="Run ANALYZE to update statistics")
    parser.add_argument("--integrity", action="store_true",
                        help="Check database integrity")
    parser.add_argument("--cleanup", action="store_true",
                        help="Clean up old data")
    parser.add_argument("--backup", type=Path,
                        help="Backup database to specified path")
    parser.add_argument("--restore", type=Path,
                        help="Restore database from backup")
    parser.add_argument("--stats", action="store_true",
                        help="Show database statistics")

    # Options
    parser.add_argument("--days", type=int, default=30,
                        help="Days threshold for cleanup (default: 30)")
    parser.add_argument("--db", type=Path,
                        default=Path(".claude/agentdb/db.sqlite"),
                        help="Database path")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Verbose output")

    args = parser.parse_args()

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    # Vérifier qu'au moins une action est spécifiée
    actions = [args.all, args.vacuum, args.analyze, args.integrity,
               args.cleanup, args.backup, args.restore, args.stats]
    if not any(actions):
        parser.print_help()
        return 1

    try:
        runner = MaintenanceRunner(args.db)

        if args.all:
            results = runner.run_all(cleanup_days=args.days)
            runner.print_stats(results["stats"])
            return 0 if results["overall"]["success"] else 1

        runner.connect()

        try:
            if args.vacuum:
                result = runner.vacuum()
                logger.info(f"VACUUM complete. Space freed: {result['space_freed']} bytes")

            if args.analyze:
                result = runner.analyze()
                logger.info("ANALYZE complete")

            if args.integrity:
                result = runner.check_integrity()
                if result["issues"]:
                    logger.warning(f"Integrity issues found: {result['issues']}")
                else:
                    logger.info("Integrity check passed")

            if args.cleanup:
                result = runner.cleanup(args.days)
                logger.info(f"Cleanup complete. Removed: {result['snapshots_deleted']} snapshots")

            if args.backup:
                result = runner.backup(args.backup)
                if result["success"]:
                    logger.info(f"Backup created: {result['backup_path']}")
                else:
                    logger.error("Backup failed")
                    return 1

            if args.restore:
                result = runner.restore(args.restore)
                if result["success"]:
                    logger.info("Restore complete")
                else:
                    logger.error("Restore failed")
                    return 1

            if args.stats:
                stats = runner.get_stats()
                runner.print_stats(stats)

        finally:
            runner.close()

        return 0

    except Exception as e:
        logger.error(f"Maintenance failed: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
