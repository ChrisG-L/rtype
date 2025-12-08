"""
AgentDB - Module de connexion et helpers pour SQLite.

Ce module gère :
- La connexion à la base SQLite avec configuration optimale (WAL, cache, etc.)
- L'initialisation du schéma
- Les transactions
- Les helpers pour les opérations courantes

Usage:
    # Connexion simple
    db = Database(".claude/agentdb/db.sqlite")
    db.connect()
    rows = db.fetch_all("SELECT * FROM files")
    db.close()

    # Avec context manager
    with Database(".claude/agentdb/db.sqlite") as db:
        with db.transaction():
            db.execute("INSERT INTO files ...")
            db.execute("UPDATE files ...")
        # Auto-commit

    # Avec DatabaseManager (recommandé)
    with DatabaseManager(".claude/agentdb/db.sqlite") as manager:
        manager.init_schema()
        manager.execute("INSERT INTO files ...")
"""

from __future__ import annotations

import logging
import sqlite3
import threading
from contextlib import contextmanager
from pathlib import Path
from typing import Any, Generator, Iterator, Optional, Union

# Configuration du logging
logger = logging.getLogger("agentdb.db")


# =============================================================================
# CONFIGURATION SQLITE
# =============================================================================

# Configuration SQLite optimale pour AgentDB
PRAGMA_CONFIG = """
PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;
PRAGMA cache_size = -64000;
PRAGMA temp_store = MEMORY;
"""

# Tables requises pour vérifier l'initialisation
REQUIRED_TABLES = [
    "files",
    "symbols",
    "relations",
    "file_relations",
    "error_history",
    "pipeline_runs",
    "snapshot_symbols",
    "patterns",
    "architecture_decisions",
    "critical_paths",
    "agentdb_meta",
]


# =============================================================================
# ROW FACTORY
# =============================================================================

def dict_factory(cursor: sqlite3.Cursor, row: tuple) -> dict[str, Any]:
    """
    Row factory pour retourner des dictionnaires au lieu de tuples.

    Args:
        cursor: Curseur SQLite
        row: Ligne de résultat

    Returns:
        Dictionnaire avec les noms de colonnes comme clés
    """
    return {col[0]: row[idx] for idx, col in enumerate(cursor.description)}


# =============================================================================
# EXCEPTIONS
# =============================================================================

class DatabaseError(Exception):
    """Erreur de base pour les opérations de base de données."""
    pass


class ConnectionError(DatabaseError):
    """Erreur de connexion à la base de données."""
    pass


class SchemaError(DatabaseError):
    """Erreur lors de l'initialisation du schéma."""
    pass


class TransactionError(DatabaseError):
    """Erreur lors d'une transaction."""
    pass


# =============================================================================
# DATABASE CLASS
# =============================================================================

class Database:
    """
    Gestionnaire de connexion SQLite pour AgentDB.

    Cette classe gère la connexion de bas niveau à SQLite avec :
    - Configuration optimale via PRAGMAs
    - Row factory pour retourner des dicts
    - Support des transactions
    - Helpers pour les opérations courantes

    Attributes:
        path: Chemin vers le fichier SQLite
        connection: Connexion SQLite active
    """

    def __init__(self, path: Union[str, Path]) -> None:
        """
        Initialise la connexion à la base de données.

        Args:
            path: Chemin vers le fichier SQLite
        """
        self.path = Path(path)
        self._connection: Optional[sqlite3.Connection] = None
        self._lock = threading.RLock()

    @property
    def connection(self) -> sqlite3.Connection:
        """Retourne la connexion active, lève une erreur si non connecté."""
        if self._connection is None:
            raise ConnectionError("Database not connected. Call connect() first.")
        return self._connection

    @property
    def is_connected(self) -> bool:
        """Vérifie si la base est connectée."""
        return self._connection is not None

    def connect(self) -> sqlite3.Connection:
        """
        Établit la connexion avec la configuration optimale.

        Crée le répertoire parent si nécessaire.
        Configure les PRAGMAs et le row factory.

        Returns:
            Connexion SQLite configurée

        Raises:
            ConnectionError: Si la connexion échoue
        """
        with self._lock:
            if self._connection is not None:
                return self._connection

            try:
                # Créer le répertoire parent si nécessaire
                self.path.parent.mkdir(parents=True, exist_ok=True)

                # Ouvrir la connexion
                self._connection = sqlite3.connect(
                    str(self.path),
                    check_same_thread=False,  # Pour usage multi-thread avec lock
                    timeout=30.0,
                )

                # Configurer le row factory pour retourner des dicts
                self._connection.row_factory = dict_factory

                # Appliquer les PRAGMAs
                self._apply_pragmas()

                logger.info(f"Connected to database: {self.path}")
                return self._connection

            except sqlite3.Error as e:
                logger.error(f"Failed to connect to database: {e}")
                raise ConnectionError(f"Failed to connect to {self.path}: {e}") from e

    def _apply_pragmas(self) -> None:
        """Applique les PRAGMAs de configuration."""
        if self._connection is None:
            return

        cursor = self._connection.cursor()
        try:
            for pragma in PRAGMA_CONFIG.strip().split("\n"):
                pragma = pragma.strip()
                if pragma and not pragma.startswith("--"):
                    cursor.execute(pragma)
            self._connection.commit()
            logger.debug("Applied SQLite PRAGMAs")
        finally:
            cursor.close()

    def close(self) -> None:
        """
        Ferme la connexion proprement.

        Effectue un checkpoint WAL avant de fermer.
        """
        with self._lock:
            if self._connection is not None:
                try:
                    # Checkpoint WAL pour persister les données
                    self._connection.execute("PRAGMA wal_checkpoint(TRUNCATE)")
                    self._connection.close()
                    logger.info(f"Closed database: {self.path}")
                except sqlite3.Error as e:
                    logger.warning(f"Error during close: {e}")
                finally:
                    self._connection = None

    def __enter__(self) -> "Database":
        """Context manager: connecte à la base."""
        self.connect()
        return self

    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> None:
        """Context manager: ferme la connexion."""
        self.close()

    # -------------------------------------------------------------------------
    # TRANSACTIONS
    # -------------------------------------------------------------------------

    @contextmanager
    def transaction(self) -> Generator[sqlite3.Cursor, None, None]:
        """
        Context manager pour les transactions.

        Usage:
            with db.transaction() as cursor:
                cursor.execute("INSERT ...")
                cursor.execute("UPDATE ...")
            # Auto-commit si pas d'erreur, rollback sinon

        Yields:
            Curseur SQLite

        Raises:
            TransactionError: Si la transaction échoue
        """
        with self._lock:
            cursor = self.connection.cursor()
            try:
                yield cursor
                self.connection.commit()
                logger.debug("Transaction committed")
            except Exception as e:
                self.connection.rollback()
                logger.error(f"Transaction rolled back: {e}")
                raise TransactionError(f"Transaction failed: {e}") from e
            finally:
                cursor.close()

    # -------------------------------------------------------------------------
    # HELPERS D'EXÉCUTION
    # -------------------------------------------------------------------------

    def execute(
        self,
        sql: str,
        params: Union[tuple, dict, None] = None,
    ) -> sqlite3.Cursor:
        """
        Exécute une requête SQL.

        Args:
            sql: Requête SQL
            params: Paramètres de la requête (tuple ou dict)

        Returns:
            Curseur avec les résultats

        Raises:
            DatabaseError: Si l'exécution échoue
        """
        with self._lock:
            try:
                cursor = self.connection.cursor()
                if params is None:
                    cursor.execute(sql)
                else:
                    cursor.execute(sql, params)
                self.connection.commit()
                return cursor
            except sqlite3.Error as e:
                logger.error(f"Execute failed: {e}\nSQL: {sql[:200]}")
                raise DatabaseError(f"Execute failed: {e}") from e

    def execute_many(
        self,
        sql: str,
        params_list: list[Union[tuple, dict]],
    ) -> int:
        """
        Exécute une requête pour plusieurs jeux de paramètres (batch insert).

        Args:
            sql: Requête SQL avec placeholders
            params_list: Liste de tuples ou dicts de paramètres

        Returns:
            Nombre de lignes affectées

        Raises:
            DatabaseError: Si l'exécution échoue
        """
        with self._lock:
            try:
                cursor = self.connection.cursor()
                cursor.executemany(sql, params_list)
                self.connection.commit()
                rows_affected = cursor.rowcount
                cursor.close()
                logger.debug(f"Batch executed: {rows_affected} rows affected")
                return rows_affected
            except sqlite3.Error as e:
                logger.error(f"Execute many failed: {e}\nSQL: {sql[:200]}")
                raise DatabaseError(f"Execute many failed: {e}") from e

    def execute_script(self, script: str) -> None:
        """
        Exécute un script SQL (plusieurs instructions).

        Args:
            script: Script SQL complet

        Raises:
            DatabaseError: Si l'exécution échoue
        """
        with self._lock:
            try:
                self.connection.executescript(script)
                logger.debug("Script executed successfully")
            except sqlite3.Error as e:
                logger.error(f"Script execution failed: {e}")
                raise DatabaseError(f"Script execution failed: {e}") from e

    # -------------------------------------------------------------------------
    # HELPERS DE LECTURE
    # -------------------------------------------------------------------------

    def fetch_one(
        self,
        sql: str,
        params: Union[tuple, dict, None] = None,
    ) -> Optional[dict[str, Any]]:
        """
        Exécute une requête et retourne une seule ligne.

        Args:
            sql: Requête SQL
            params: Paramètres

        Returns:
            Dictionnaire de la ligne ou None si pas de résultat
        """
        with self._lock:
            try:
                cursor = self.connection.cursor()
                if params is None:
                    cursor.execute(sql)
                else:
                    cursor.execute(sql, params)
                row = cursor.fetchone()
                cursor.close()
                return row
            except sqlite3.Error as e:
                logger.error(f"Fetch one failed: {e}\nSQL: {sql[:200]}")
                raise DatabaseError(f"Fetch one failed: {e}") from e

    def fetch_all(
        self,
        sql: str,
        params: Union[tuple, dict, None] = None,
    ) -> list[dict[str, Any]]:
        """
        Exécute une requête et retourne toutes les lignes.

        Args:
            sql: Requête SQL
            params: Paramètres

        Returns:
            Liste de dictionnaires
        """
        with self._lock:
            try:
                cursor = self.connection.cursor()
                if params is None:
                    cursor.execute(sql)
                else:
                    cursor.execute(sql, params)
                rows = cursor.fetchall()
                cursor.close()
                return rows
            except sqlite3.Error as e:
                logger.error(f"Fetch all failed: {e}\nSQL: {sql[:200]}")
                raise DatabaseError(f"Fetch all failed: {e}") from e

    def fetch_iter(
        self,
        sql: str,
        params: Union[tuple, dict, None] = None,
        batch_size: int = 1000,
    ) -> Iterator[dict[str, Any]]:
        """
        Exécute une requête et retourne un itérateur sur les lignes.

        Utile pour les grandes requêtes qui ne tiennent pas en mémoire.

        Args:
            sql: Requête SQL
            params: Paramètres
            batch_size: Nombre de lignes à charger par batch

        Yields:
            Dictionnaires de lignes
        """
        with self._lock:
            try:
                cursor = self.connection.cursor()
                if params is None:
                    cursor.execute(sql)
                else:
                    cursor.execute(sql, params)

                while True:
                    rows = cursor.fetchmany(batch_size)
                    if not rows:
                        break
                    for row in rows:
                        yield row

                cursor.close()
            except sqlite3.Error as e:
                logger.error(f"Fetch iter failed: {e}\nSQL: {sql[:200]}")
                raise DatabaseError(f"Fetch iter failed: {e}") from e

    def fetch_scalar(
        self,
        sql: str,
        params: Union[tuple, dict, None] = None,
    ) -> Any:
        """
        Exécute une requête et retourne la première colonne de la première ligne.

        Utile pour les COUNT, MAX, etc.

        Args:
            sql: Requête SQL
            params: Paramètres

        Returns:
            Valeur scalaire ou None
        """
        row = self.fetch_one(sql, params)
        if row:
            return list(row.values())[0]
        return None


# =============================================================================
# DATABASE MANAGER
# =============================================================================

class DatabaseManager(Database):
    """
    Gestionnaire complet de base de données AgentDB.

    Étend Database avec :
    - Initialisation du schéma
    - Vérification d'intégrité
    - Gestion des versions
    - Utilitaires de maintenance

    Usage:
        with DatabaseManager(".claude/agentdb/db.sqlite") as manager:
            manager.init_schema()
            # Utiliser la base...
    """

    def __init__(
        self,
        db_path: Union[str, Path],
        schema_path: Optional[Union[str, Path]] = None,
    ) -> None:
        """
        Initialise le manager.

        Args:
            db_path: Chemin vers le fichier SQLite
            schema_path: Chemin vers schema.sql (défaut: même dossier que db)
        """
        super().__init__(db_path)

        if schema_path is None:
            self.schema_path = self.path.parent / "schema.sql"
        else:
            self.schema_path = Path(schema_path)

    # -------------------------------------------------------------------------
    # INITIALISATION DU SCHÉMA
    # -------------------------------------------------------------------------

    def init_schema(self, force: bool = False) -> bool:
        """
        Crée les tables à partir du fichier schema.sql.

        Args:
            force: Si True, recrée les tables même si elles existent

        Returns:
            True si le schéma a été créé, False s'il existait déjà

        Raises:
            SchemaError: Si le fichier schema.sql n'existe pas ou si l'exécution échoue
        """
        # Vérifier si déjà initialisé
        if not force and self.is_initialized():
            logger.info("Database already initialized, skipping schema creation")
            return False

        # Vérifier que le fichier schema.sql existe
        if not self.schema_path.exists():
            raise SchemaError(f"Schema file not found: {self.schema_path}")

        try:
            # Lire et exécuter le schéma
            schema_sql = self.schema_path.read_text(encoding="utf-8")
            self.execute_script(schema_sql)

            logger.info(f"Schema initialized from: {self.schema_path}")
            return True

        except Exception as e:
            logger.error(f"Failed to initialize schema: {e}")
            raise SchemaError(f"Failed to initialize schema: {e}") from e

    def is_initialized(self) -> bool:
        """
        Vérifie si la base est initialisée (tables créées).

        Returns:
            True si toutes les tables requises existent
        """
        try:
            # Récupérer la liste des tables existantes
            rows = self.fetch_all(
                "SELECT name FROM sqlite_master WHERE type='table'"
            )
            existing_tables = {row["name"] for row in rows}

            # Vérifier que toutes les tables requises existent
            for table in REQUIRED_TABLES:
                if table not in existing_tables:
                    return False

            return True

        except Exception:
            return False

    def get_schema_version(self) -> Optional[str]:
        """
        Retourne la version du schéma depuis agentdb_meta.

        Returns:
            Version du schéma (ex: "2.0") ou None si non initialisé
        """
        try:
            row = self.fetch_one(
                "SELECT value FROM agentdb_meta WHERE key = 'schema_version'"
            )
            if row:
                return row["value"]
            return None
        except Exception:
            return None

    def set_meta(self, key: str, value: str) -> None:
        """
        Définit une valeur dans agentdb_meta.

        Args:
            key: Clé de métadonnée
            value: Valeur
        """
        self.execute(
            """
            INSERT INTO agentdb_meta (key, value, updated_at)
            VALUES (?, ?, datetime('now'))
            ON CONFLICT(key) DO UPDATE SET
                value = excluded.value,
                updated_at = excluded.updated_at
            """,
            (key, value),
        )

    def get_meta(self, key: str, default: Optional[str] = None) -> Optional[str]:
        """
        Récupère une valeur depuis agentdb_meta.

        Args:
            key: Clé de métadonnée
            default: Valeur par défaut si non trouvée

        Returns:
            Valeur ou default
        """
        row = self.fetch_one(
            "SELECT value FROM agentdb_meta WHERE key = ?",
            (key,),
        )
        if row:
            return row["value"]
        return default

    # -------------------------------------------------------------------------
    # STATISTIQUES ET MAINTENANCE
    # -------------------------------------------------------------------------

    def get_stats(self) -> dict[str, int]:
        """
        Retourne les statistiques de la base.

        Returns:
            Dictionnaire avec le nombre d'entrées par table
        """
        stats = {}
        for table in REQUIRED_TABLES:
            try:
                count = self.fetch_scalar(f"SELECT COUNT(*) FROM {table}")
                stats[table] = count or 0
            except Exception:
                stats[table] = -1
        return stats

    def vacuum(self) -> None:
        """
        Compacte la base de données (VACUUM).

        Libère l'espace inutilisé et optimise les performances.
        """
        logger.info("Running VACUUM...")
        self.execute("VACUUM")
        logger.info("VACUUM completed")

    def analyze(self) -> None:
        """
        Met à jour les statistiques de la base (ANALYZE).

        Améliore les performances des requêtes.
        """
        logger.info("Running ANALYZE...")
        self.execute("ANALYZE")
        logger.info("ANALYZE completed")

    def integrity_check(self) -> tuple[bool, list[str]]:
        """
        Vérifie l'intégrité de la base de données.

        Returns:
            Tuple (is_ok, errors): True si OK, liste des erreurs sinon
        """
        try:
            rows = self.fetch_all("PRAGMA integrity_check")
            errors = [row["integrity_check"] for row in rows if row["integrity_check"] != "ok"]

            if errors:
                logger.warning(f"Integrity check found {len(errors)} issues")
                return False, errors

            logger.info("Integrity check passed")
            return True, []

        except Exception as e:
            return False, [str(e)]

    def foreign_key_check(self) -> tuple[bool, list[dict]]:
        """
        Vérifie les contraintes de clés étrangères.

        Returns:
            Tuple (is_ok, violations): True si OK, liste des violations sinon
        """
        try:
            rows = self.fetch_all("PRAGMA foreign_key_check")

            if rows:
                logger.warning(f"Foreign key check found {len(rows)} violations")
                return False, rows

            logger.info("Foreign key check passed")
            return True, []

        except Exception as e:
            return False, [{"error": str(e)}]

    def get_table_info(self, table: str) -> list[dict[str, Any]]:
        """
        Retourne les informations sur les colonnes d'une table.

        Args:
            table: Nom de la table

        Returns:
            Liste des colonnes avec leurs propriétés
        """
        return self.fetch_all(f"PRAGMA table_info({table})")

    def get_index_list(self, table: str) -> list[dict[str, Any]]:
        """
        Retourne la liste des index d'une table.

        Args:
            table: Nom de la table

        Returns:
            Liste des index
        """
        return self.fetch_all(f"PRAGMA index_list({table})")

    def backup(self, backup_path: Union[str, Path]) -> None:
        """
        Crée une sauvegarde de la base de données.

        Args:
            backup_path: Chemin du fichier de sauvegarde
        """
        backup_path = Path(backup_path)
        backup_path.parent.mkdir(parents=True, exist_ok=True)

        logger.info(f"Creating backup: {backup_path}")

        with sqlite3.connect(str(backup_path)) as backup_conn:
            self.connection.backup(backup_conn)

        logger.info("Backup completed")


# =============================================================================
# FONCTIONS UTILITAIRES
# =============================================================================

def get_database(
    db_path: Optional[Union[str, Path]] = None,
) -> DatabaseManager:
    """
    Factory function pour obtenir une instance de DatabaseManager.

    Args:
        db_path: Chemin vers la base (défaut: .claude/agentdb/db.sqlite)

    Returns:
        Instance de DatabaseManager connectée
    """
    if db_path is None:
        db_path = Path(".claude/agentdb/db.sqlite")

    manager = DatabaseManager(db_path)
    manager.connect()
    return manager
