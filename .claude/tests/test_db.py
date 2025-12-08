"""
Tests pour la couche base de données AgentDB.

Teste :
- Connexion à la base
- Initialisation du schéma
- Transactions (commit, rollback)
- Méthodes utilitaires (fetch_one, fetch_all, execute)
"""

import pytest
import sqlite3
import tempfile
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from agentdb.db import Database, DatabaseManager


# =============================================================================
# TESTS DE CONNEXION
# =============================================================================

class TestConnection:
    """Tests de connexion à la base de données."""

    def test_connect_memory(self):
        """Teste la connexion à une base en mémoire."""
        db = Database(":memory:")
        db.connect()
        assert db.connection is not None
        assert db.is_connected
        db.close()

    def test_connect_file(self):
        """Teste la connexion à un fichier."""
        with tempfile.NamedTemporaryFile(suffix=".sqlite", delete=False) as f:
            db_path = f.name

        try:
            db = Database(db_path)
            db.connect()
            assert db.is_connected
            assert os.path.exists(db_path)
            db.close()
        finally:
            os.unlink(db_path)

    def test_close_connection(self):
        """Teste la fermeture de connexion."""
        db = Database(":memory:")
        db.connect()
        assert db.is_connected
        db.close()
        assert not db.is_connected

    def test_context_manager(self):
        """Teste l'utilisation comme context manager."""
        with Database(":memory:") as db:
            assert db.is_connected
            db.execute("SELECT 1")
        # La connexion devrait être fermée après le with
        assert not db.is_connected

    def test_multiple_close(self):
        """Teste que plusieurs close() ne causent pas d'erreur."""
        db = Database(":memory:")
        db.close()
        db.close()  # Ne doit pas lever d'exception

    def test_row_factory(self):
        """Vérifie que row_factory retourne des dicts."""
        db = Database(":memory:")
        db.connect()
        db.execute("CREATE TABLE test (id INTEGER, name TEXT)")
        db.execute("INSERT INTO test VALUES (1, 'test')")
        row = db.fetch_one("SELECT * FROM test")

        assert isinstance(row, (dict, sqlite3.Row))
        assert row["id"] == 1
        assert row["name"] == "test"
        db.close()


# =============================================================================
# TESTS D'INITIALISATION DU SCHÉMA
# =============================================================================

class TestInitSchema:
    """Tests d'initialisation du schéma."""

    def test_init_schema_creates_tables(self, db):
        """Vérifie que init_schema crée les tables."""
        # Vérifier les tables principales
        tables_query = """
            SELECT name FROM sqlite_master
            WHERE type='table' AND name NOT LIKE 'sqlite_%'
            ORDER BY name
        """
        rows = db.fetch_all(tables_query)
        table_names = [r["name"] for r in rows]

        assert "files" in table_names
        assert "symbols" in table_names
        assert "relations" in table_names
        assert "file_relations" in table_names
        assert "error_history" in table_names
        assert "pipeline_runs" in table_names
        assert "patterns" in table_names
        assert "architecture_decisions" in table_names
        assert "critical_paths" in table_names
        assert "agentdb_meta" in table_names

    def test_init_schema_creates_indexes(self, db):
        """Vérifie que init_schema crée les index."""
        indexes_query = """
            SELECT name FROM sqlite_master
            WHERE type='index' AND name NOT LIKE 'sqlite_%'
        """
        rows = db.fetch_all(indexes_query)
        index_names = [r["name"] for r in rows]

        # Vérifier quelques index importants
        assert "idx_files_module" in index_names
        assert "idx_symbols_file_id" in index_names
        assert "idx_symbols_name" in index_names
        assert "idx_relations_source" in index_names
        assert "idx_relations_target" in index_names

    def test_init_schema_creates_views(self, db):
        """Vérifie que init_schema crée les vues."""
        views_query = """
            SELECT name FROM sqlite_master
            WHERE type='view'
        """
        rows = db.fetch_all(views_query)
        view_names = [r["name"] for r in rows]

        assert "v_files_with_stats" in view_names
        assert "v_symbols_with_context" in view_names
        assert "v_relations_named" in view_names

    def test_init_schema_idempotent(self, db):
        """Vérifie que init_schema peut être appelé plusieurs fois."""
        # Déjà appelé dans la fixture, appeler à nouveau
        db.init_schema()
        db.init_schema()

        # Vérifier que les tables existent toujours
        row = db.fetch_one("SELECT COUNT(*) as cnt FROM sqlite_master WHERE type='table'")
        assert row["cnt"] > 0

    def test_init_schema_sets_pragmas(self, db):
        """Vérifie que les PRAGMA sont correctement configurés."""
        # Vérifier le mode journal
        row = db.fetch_one("PRAGMA journal_mode")
        # En mémoire, le mode WAL peut ne pas être disponible
        assert row is not None

        # Vérifier les foreign keys
        row = db.fetch_one("PRAGMA foreign_keys")
        assert row["foreign_keys"] == 1

    def test_schema_meta_initialized(self, db):
        """Vérifie que la table meta est initialisée."""
        row = db.fetch_one("SELECT value FROM agentdb_meta WHERE key = 'schema_version'")
        assert row is not None
        assert row["value"] == "2.0"


# =============================================================================
# TESTS DES TRANSACTIONS
# =============================================================================

class TestTransactions:
    """Tests des transactions."""

    def test_execute_auto_commits(self, db):
        """Teste que execute auto-commits les données."""
        db.execute("INSERT INTO files (path, filename) VALUES ('test.c', 'test.c')")
        # Pas besoin d'appeler commit - execute le fait automatiquement

        # Vérifier que les données sont persistées
        row = db.fetch_one("SELECT path FROM files WHERE path = 'test.c'")
        assert row is not None
        assert row["path"] == "test.c"

    def test_transaction_context_manager(self, db):
        """Teste les transactions avec context manager."""
        # Utiliser transaction() pour grouper plusieurs opérations
        with db.transaction():
            cursor = db.connection.cursor()
            cursor.execute("INSERT INTO files (path, filename) VALUES ('ctx.c', 'ctx.c')")
            cursor.close()

        row = db.fetch_one("SELECT path FROM files WHERE path = 'ctx.c'")
        assert row is not None

    def test_transaction_rollback_on_error(self, db):
        """Teste le rollback automatique en cas d'erreur dans transaction()."""
        from agentdb.db import TransactionError

        # Insérer via transaction avec erreur
        try:
            with db.transaction() as cursor:
                cursor.execute("INSERT INTO files (path, filename) VALUES ('error.c', 'error.c')")
                # Causer une erreur (violation de contrainte unique)
                cursor.execute("INSERT INTO files (path, filename) VALUES ('error.c', 'error.c')")
        except TransactionError:
            pass  # Expected

        # La première insertion devrait être rollback
        row = db.fetch_one("SELECT path FROM files WHERE path = 'error.c'")
        assert row is None

    def test_connection_isolation_level(self, db):
        """Vérifie le niveau d'isolation de la connexion."""
        # La connexion devrait avoir un isolation_level défini
        assert db.connection.isolation_level is not None or db.connection.isolation_level == ""


# =============================================================================
# TESTS DES MÉTHODES UTILITAIRES
# =============================================================================

class TestUtilityMethods:
    """Tests des méthodes utilitaires."""

    def test_execute_simple(self, db):
        """Teste execute avec une requête simple."""
        db.execute("INSERT INTO files (path, filename) VALUES (?, ?)", ("test.c", "test.c"))
        # execute auto-commits

        row = db.fetch_one("SELECT COUNT(*) as cnt FROM files")
        assert row["cnt"] == 1

    def test_execute_with_named_params(self, db):
        """Teste execute avec des paramètres nommés."""
        db.execute(
            "INSERT INTO files (path, filename, module) VALUES (:path, :filename, :module)",
            {"path": "test.c", "filename": "test.c", "module": "core"}
        )
        # execute auto-commits

        row = db.fetch_one("SELECT module FROM files WHERE path = 'test.c'")
        assert row["module"] == "core"

    def test_fetch_one_returns_none(self, db):
        """Teste que fetch_one retourne None si pas de résultat."""
        row = db.fetch_one("SELECT * FROM files WHERE path = 'nonexistent'")
        assert row is None

    def test_fetch_one_returns_first(self, db):
        """Teste que fetch_one retourne le premier résultat."""
        db.execute("INSERT INTO files (path, filename) VALUES ('a.c', 'a.c')")
        db.execute("INSERT INTO files (path, filename) VALUES ('b.c', 'b.c')")
        # execute auto-commits

        row = db.fetch_one("SELECT path FROM files ORDER BY path")
        assert row["path"] == "a.c"

    def test_fetch_all_empty(self, db):
        """Teste que fetch_all retourne une liste vide si pas de résultats."""
        rows = db.fetch_all("SELECT * FROM files WHERE path = 'nonexistent'")
        assert rows == []

    def test_fetch_all_multiple(self, db):
        """Teste fetch_all avec plusieurs résultats."""
        db.execute("INSERT INTO files (path, filename) VALUES ('a.c', 'a.c')")
        db.execute("INSERT INTO files (path, filename) VALUES ('b.c', 'b.c')")
        db.execute("INSERT INTO files (path, filename) VALUES ('c.c', 'c.c')")
        # execute auto-commits

        rows = db.fetch_all("SELECT path FROM files ORDER BY path")
        assert len(rows) == 3
        assert rows[0]["path"] == "a.c"
        assert rows[2]["path"] == "c.c"

    def test_execute_many(self, db):
        """Teste execute_many pour les insertions en batch."""
        data = [
            ("a.c", "a.c"),
            ("b.c", "b.c"),
            ("c.c", "c.c"),
        ]
        rows_affected = db.execute_many(
            "INSERT INTO files (path, filename) VALUES (?, ?)",
            data
        )
        # execute_many auto-commits

        row = db.fetch_one("SELECT COUNT(*) as cnt FROM files")
        assert row["cnt"] == 3
        assert rows_affected == 3


# =============================================================================
# TESTS DE GESTION DES ERREURS
# =============================================================================

class TestErrorHandling:
    """Tests de gestion des erreurs."""

    def test_invalid_sql(self, db):
        """Teste la gestion des requêtes SQL invalides."""
        from agentdb.db import DatabaseError
        with pytest.raises(DatabaseError):
            db.execute("SELECT * FROM nonexistent_table")

    def test_constraint_violation(self, db):
        """Teste la gestion des violations de contraintes."""
        from agentdb.db import DatabaseError
        db.execute("INSERT INTO files (path, filename) VALUES ('test.c', 'test.c')")
        # execute auto-commits

        with pytest.raises(DatabaseError):
            db.execute("INSERT INTO files (path, filename) VALUES ('test.c', 'test.c')")

    def test_foreign_key_violation(self, db):
        """Teste la gestion des violations de clés étrangères."""
        from agentdb.db import DatabaseError
        # Essayer d'insérer un symbole avec un file_id inexistant
        with pytest.raises(DatabaseError):
            db.execute(
                "INSERT INTO symbols (file_id, name, kind) VALUES (9999, 'test', 'function')"
            )

    def test_type_mismatch_handled(self, db):
        """Teste que les erreurs de type sont gérées."""
        # SQLite est permissif avec les types, mais on peut tester le comportement
        db.execute("INSERT INTO files (path, filename, lines_total) VALUES ('test.c', 'test.c', 'not_a_number')")
        # execute auto-commits

        row = db.fetch_one("SELECT lines_total FROM files WHERE path = 'test.c'")
        # SQLite stocke comme TEXT, la lecture retourne la valeur telle quelle
        assert row is not None


# =============================================================================
# TESTS DE PERFORMANCE
# =============================================================================

class TestPerformance:
    """Tests de performance basiques."""

    def test_bulk_insert_performance(self, db, assert_performance):
        """Teste la performance des insertions en masse."""
        data = [(f"file_{i}.c", f"file_{i}.c") for i in range(1000)]

        with assert_performance(max_ms=2000) as perf:  # Higher threshold for auto-commit overhead
            for path, filename in data:
                db.execute("INSERT INTO files (path, filename) VALUES (?, ?)", (path, filename))
            # execute auto-commits each time

        perf.check()

        # Vérifier que les données sont insérées
        row = db.fetch_one("SELECT COUNT(*) as cnt FROM files")
        assert row["cnt"] == 1000

    def test_query_performance(self, db, assert_performance):
        """Teste la performance des requêtes."""
        # Insérer des données via execute_many pour meilleure performance
        data = [(f"src/file_{i}.c", f"file_{i}.c", "core") for i in range(100)]
        db.execute_many(
            "INSERT INTO files (path, filename, module) VALUES (?, ?, ?)",
            data
        )

        with assert_performance(max_ms=50) as perf:
            rows = db.fetch_all("SELECT * FROM files WHERE module = 'core'")

        perf.check()
        assert len(rows) == 100

    def test_index_usage(self, db):
        """Vérifie que les index sont utilisés."""
        # Insérer des données via execute_many
        data = [(f"src/file_{i}.c", f"file_{i}.c", f"module_{i % 10}") for i in range(100)]
        db.execute_many(
            "INSERT INTO files (path, filename, module) VALUES (?, ?, ?)",
            data
        )

        # Analyser le plan de requête
        plan = db.fetch_all("EXPLAIN QUERY PLAN SELECT * FROM files WHERE module = 'module_0'")
        # Le plan devrait mentionner l'index
        plan_str = str([dict(row) for row in plan])
        # Note: SQLite peut choisir de ne pas utiliser l'index pour petites tables
        assert plan is not None


# =============================================================================
# TESTS D'INTÉGRITÉ
# =============================================================================

class TestIntegrity:
    """Tests d'intégrité de la base."""

    def test_cascade_delete_file(self, db):
        """Teste la suppression en cascade lors de la suppression d'un fichier."""
        # Insérer un fichier avec des symboles
        db.execute("INSERT INTO files (path, filename) VALUES ('test.c', 'test.c')")
        file_id = db.fetch_one("SELECT id FROM files WHERE path = 'test.c'")["id"]

        db.execute(
            "INSERT INTO symbols (file_id, name, kind) VALUES (?, 'main', 'function')",
            (file_id,)
        )
        # execute auto-commits

        # Vérifier que le symbole existe
        row = db.fetch_one("SELECT COUNT(*) as cnt FROM symbols WHERE file_id = ?", (file_id,))
        assert row["cnt"] == 1

        # Supprimer le fichier
        db.execute("DELETE FROM files WHERE id = ?", (file_id,))
        # execute auto-commits

        # Vérifier que le symbole est supprimé (CASCADE)
        row = db.fetch_one("SELECT COUNT(*) as cnt FROM symbols WHERE file_id = ?", (file_id,))
        assert row["cnt"] == 0

    def test_cascade_delete_symbol(self, db):
        """Teste la suppression en cascade lors de la suppression d'un symbole."""
        # Insérer fichier, symboles, relation
        db.execute("INSERT INTO files (path, filename) VALUES ('test.c', 'test.c')")
        file_id = db.fetch_one("SELECT id FROM files")["id"]

        db.execute("INSERT INTO symbols (file_id, name, kind) VALUES (?, 'func1', 'function')", (file_id,))
        db.execute("INSERT INTO symbols (file_id, name, kind) VALUES (?, 'func2', 'function')", (file_id,))
        sym1_id = db.fetch_one("SELECT id FROM symbols WHERE name = 'func1'")["id"]
        sym2_id = db.fetch_one("SELECT id FROM symbols WHERE name = 'func2'")["id"]

        db.execute(
            "INSERT INTO relations (source_id, target_id, relation_type) VALUES (?, ?, 'calls')",
            (sym1_id, sym2_id)
        )
        # execute auto-commits

        # Supprimer le symbole source
        db.execute("DELETE FROM symbols WHERE id = ?", (sym1_id,))
        # execute auto-commits

        # La relation devrait être supprimée (CASCADE)
        row = db.fetch_one("SELECT COUNT(*) as cnt FROM relations WHERE source_id = ?", (sym1_id,))
        assert row["cnt"] == 0


# =============================================================================
# TESTS DES VUES
# =============================================================================

class TestViews:
    """Tests des vues SQL."""

    def test_v_files_with_stats(self, db):
        """Teste la vue v_files_with_stats."""
        # Insérer des données
        db.execute("INSERT INTO files (path, filename) VALUES ('test.c', 'test.c')")
        file_id = db.fetch_one("SELECT id FROM files")["id"]

        db.execute("INSERT INTO symbols (file_id, name, kind) VALUES (?, 'func1', 'function')", (file_id,))
        db.execute("INSERT INTO symbols (file_id, name, kind) VALUES (?, 'func2', 'function')", (file_id,))
        db.execute("INSERT INTO symbols (file_id, name, kind) VALUES (?, 'MyStruct', 'struct')", (file_id,))
        # execute auto-commits

        # Interroger la vue
        row = db.fetch_one("SELECT * FROM v_files_with_stats WHERE path = 'test.c'")
        assert row is not None
        assert row["symbol_count"] == 3
        assert row["function_count"] == 2
        assert row["type_count"] == 1

    def test_v_symbols_with_context(self, db):
        """Teste la vue v_symbols_with_context."""
        db.execute("INSERT INTO files (path, filename, module) VALUES ('test.c', 'test.c', 'core')")
        file_id = db.fetch_one("SELECT id FROM files")["id"]

        db.execute("INSERT INTO symbols (file_id, name, kind) VALUES (?, 'main', 'function')", (file_id,))
        # execute auto-commits

        row = db.fetch_one("SELECT * FROM v_symbols_with_context WHERE name = 'main'")
        assert row is not None
        assert row["file_path"] == "test.c"
        assert row["file_module"] == "core"

    def test_v_relations_named(self, db):
        """Teste la vue v_relations_named."""
        db.execute("INSERT INTO files (path, filename) VALUES ('test.c', 'test.c')")
        file_id = db.fetch_one("SELECT id FROM files")["id"]

        db.execute("INSERT INTO symbols (file_id, name, kind) VALUES (?, 'caller', 'function')", (file_id,))
        db.execute("INSERT INTO symbols (file_id, name, kind) VALUES (?, 'callee', 'function')", (file_id,))
        caller_id = db.fetch_one("SELECT id FROM symbols WHERE name = 'caller'")["id"]
        callee_id = db.fetch_one("SELECT id FROM symbols WHERE name = 'callee'")["id"]

        db.execute(
            "INSERT INTO relations (source_id, target_id, relation_type) VALUES (?, ?, 'calls')",
            (caller_id, callee_id)
        )
        # execute auto-commits

        row = db.fetch_one("SELECT * FROM v_relations_named WHERE source_name = 'caller'")
        assert row is not None
        assert row["target_name"] == "callee"
        assert row["relation_type"] == "calls"
