"""
Fixtures partagées pour les tests AgentDB.

Ce module contient les fixtures pytest réutilisables :
- db : Base de données en mémoire
- populated_db : Base avec données de test
- sample_files, sample_symbols, sample_relations : Données de test
"""

import pytest
import sqlite3
from pathlib import Path
from datetime import datetime, timedelta
import sys

# Ajouter le répertoire parent au path pour les imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from agentdb.db import Database, DatabaseManager
from agentdb.models import (
    File, Symbol, Relation, FileRelation,
    ErrorHistory, PipelineRun, Pattern, ArchitectureDecision,
)


# =============================================================================
# FIXTURES DE BASE
# =============================================================================

@pytest.fixture
def db():
    """Crée une base de données en mémoire avec le schéma initialisé."""
    # Trouver le chemin du schema.sql
    schema_path = Path(__file__).parent.parent / "agentdb" / "schema.sql"
    database = DatabaseManager(":memory:", schema_path=schema_path)
    database.connect()
    database.init_schema()
    yield database
    database.close()


@pytest.fixture
def raw_connection():
    """Connexion SQLite brute pour tests bas niveau."""
    conn = sqlite3.connect(":memory:")
    conn.row_factory = sqlite3.Row
    yield conn
    conn.close()


# =============================================================================
# DONNÉES DE TEST
# =============================================================================

@pytest.fixture
def sample_files():
    """Fichiers de test."""
    return [
        File(
            path="src/main.c",
            filename="main.c",
            extension=".c",
            module="core",
            language="c",
            is_critical=True,
            lines_total=100,
            lines_code=80,
            lines_comment=15,
            lines_blank=5,
            complexity_avg=5.0,
            complexity_max=10,
        ),
        File(
            path="src/lcd/init.c",
            filename="init.c",
            extension=".c",
            module="lcd",
            language="c",
            is_critical=False,
            lines_total=200,
            lines_code=150,
            lines_comment=30,
            lines_blank=20,
            complexity_avg=8.0,
            complexity_max=15,
        ),
        File(
            path="src/lcd/lcd.h",
            filename="lcd.h",
            extension=".h",
            module="lcd",
            language="c",
            is_critical=False,
            lines_total=50,
            lines_code=30,
            lines_comment=15,
            lines_blank=5,
        ),
        File(
            path="src/utils/helper.c",
            filename="helper.c",
            extension=".c",
            module="utils",
            language="c",
            is_critical=False,
            lines_total=80,
            lines_code=60,
            lines_comment=10,
            lines_blank=10,
        ),
        File(
            path="tests/test_lcd.c",
            filename="test_lcd.c",
            extension=".c",
            module="tests",
            language="c",
            file_type="test",
            lines_total=150,
            lines_code=120,
        ),
    ]


@pytest.fixture
def sample_symbols():
    """Symboles de test (nécessite les IDs de fichiers)."""
    return [
        # Fichier 1 : main.c
        {"file_id": 1, "name": "main", "kind": "function", "signature": "int main(int argc, char **argv)", "line_start": 10, "line_end": 50, "complexity": 5},
        {"file_id": 1, "name": "system_init", "kind": "function", "signature": "void system_init(void)", "line_start": 55, "line_end": 80, "complexity": 8},

        # Fichier 2 : lcd/init.c
        {"file_id": 2, "name": "lcd_init", "kind": "function", "signature": "int lcd_init(LCD_Config *config)", "line_start": 20, "line_end": 80, "complexity": 12},
        {"file_id": 2, "name": "lcd_reset", "kind": "function", "signature": "void lcd_reset(void)", "line_start": 85, "line_end": 100, "complexity": 3},
        {"file_id": 2, "name": "LCD_Config", "kind": "struct", "line_start": 5, "line_end": 15},

        # Fichier 3 : lcd/lcd.h
        {"file_id": 3, "name": "LCD_Config", "kind": "struct", "line_start": 10, "line_end": 20},
        {"file_id": 3, "name": "LCD_INIT", "kind": "macro", "line_start": 5},

        # Fichier 4 : utils/helper.c
        {"file_id": 4, "name": "helper_alloc", "kind": "function", "signature": "void *helper_alloc(size_t size)", "line_start": 10, "line_end": 25, "complexity": 4},
        {"file_id": 4, "name": "helper_free", "kind": "function", "signature": "void helper_free(void *ptr)", "line_start": 30, "line_end": 40, "complexity": 2},
    ]


@pytest.fixture
def sample_relations():
    """Relations de test (nécessite les IDs de symboles)."""
    return [
        # main -> system_init
        {"source_id": 1, "target_id": 2, "relation_type": "calls", "location_line": 25},
        # system_init -> lcd_init
        {"source_id": 2, "target_id": 3, "relation_type": "calls", "location_line": 60},
        # lcd_init -> lcd_reset
        {"source_id": 3, "target_id": 4, "relation_type": "calls", "location_line": 50},
        # lcd_init -> helper_alloc
        {"source_id": 3, "target_id": 8, "relation_type": "calls", "location_line": 35},
        # lcd_init uses LCD_Config
        {"source_id": 3, "target_id": 5, "relation_type": "uses_type", "location_line": 20},
        # helper_alloc -> helper_free (cleanup)
        {"source_id": 8, "target_id": 9, "relation_type": "calls", "location_line": 20},
    ]


@pytest.fixture
def sample_file_relations():
    """Relations entre fichiers."""
    return [
        # lcd/init.c includes lcd/lcd.h
        {"source_file_id": 2, "target_file_id": 3, "relation_type": "includes", "line_number": 1},
        # main.c includes lcd/lcd.h
        {"source_file_id": 1, "target_file_id": 3, "relation_type": "includes", "line_number": 3},
        # test_lcd.c includes lcd/lcd.h
        {"source_file_id": 5, "target_file_id": 3, "relation_type": "includes", "line_number": 1},
    ]


@pytest.fixture
def sample_errors():
    """Erreurs de test."""
    now = datetime.now()
    return [
        {
            "file_path": "src/lcd/init.c",
            "symbol_name": "lcd_init",
            "error_type": "memory_leak",
            "severity": "high",
            "title": "Memory leak in lcd_init",
            "description": "Buffer not freed on error path",
            "discovered_at": (now - timedelta(days=10)).isoformat(),
            "resolved_at": (now - timedelta(days=5)).isoformat(),
            "resolution": "Added cleanup in error handler",
        },
        {
            "file_path": "src/lcd/init.c",
            "error_type": "null_pointer",
            "severity": "critical",
            "title": "Null pointer dereference",
            "discovered_at": (now - timedelta(days=30)).isoformat(),
            "is_regression": True,
        },
        {
            "file_path": "src/main.c",
            "error_type": "logic_error",
            "severity": "medium",
            "title": "Off-by-one error",
            "discovered_at": (now - timedelta(days=60)).isoformat(),
            "resolved_at": (now - timedelta(days=55)).isoformat(),
        },
    ]


@pytest.fixture
def sample_patterns():
    """Patterns de test."""
    return [
        {
            "name": "error_check_return",
            "category": "error_handling",
            "title": "Check Return Values",
            "description": "Always check return values of functions that can fail",
            "severity": "high",
            "scope": "project",
            "good_example": "if ((ret = func()) != 0) return ret;",
            "bad_example": "func(); // ignoring return",
            "is_active": True,
        },
        {
            "name": "lcd_naming",
            "category": "naming_convention",
            "title": "LCD Module Naming",
            "description": "LCD functions must start with lcd_",
            "severity": "warning",
            "scope": "module",
            "module": "lcd",
            "is_active": True,
        },
        {
            "name": "deprecated_pattern",
            "category": "documentation",
            "title": "Deprecated Pattern",
            "description": "This pattern is no longer active",
            "severity": "low",
            "is_active": False,
        },
    ]


@pytest.fixture
def sample_adrs():
    """ADRs de test."""
    return [
        {
            "decision_id": "ADR-001",
            "title": "Use SQLite for local storage",
            "status": "accepted",
            "context": "We need a local database for the agent system",
            "decision": "Use SQLite with WAL mode",
            "consequences": "Simple deployment, good performance for read-heavy workloads",
            "date_decided": "2024-01-15",
            "decided_by": "Tech Lead",
            "affected_modules_json": '["core", "lcd"]',
        },
        {
            "decision_id": "ADR-002",
            "title": "Error Handling Strategy",
            "status": "accepted",
            "context": "Need consistent error handling across modules",
            "decision": "Use return codes for C functions, exceptions for Python",
            "consequences": "Clear error propagation, testable code",
            "date_decided": "2024-01-20",
            "decided_by": "Tech Lead",
        },
        {
            "decision_id": "ADR-003",
            "title": "Old Decision",
            "status": "deprecated",
            "context": "Old context",
            "decision": "Old decision",
            "date_decided": "2023-01-01",
        },
    ]


# =============================================================================
# FIXTURE AVEC DONNÉES PEUPLÉES
# =============================================================================

@pytest.fixture
def populated_db(db, sample_files, sample_symbols, sample_relations,
                 sample_file_relations, sample_errors, sample_patterns, sample_adrs):
    """Base de données avec toutes les données de test."""
    from agentdb.crud import (
        FileRepository, SymbolRepository, RelationRepository,
        FileRelationRepository, ErrorHistoryRepository, PatternRepository,
        ArchitectureDecisionRepository,
    )

    # Insérer les fichiers
    file_repo = FileRepository(db)
    for f in sample_files:
        file_repo.create(f)

    # Insérer les symboles
    for sym_data in sample_symbols:
        db.execute(
            """
            INSERT INTO symbols (file_id, name, kind, signature, line_start, line_end, complexity)
            VALUES (?, ?, ?, ?, ?, ?, ?)
            """,
            (
                sym_data["file_id"],
                sym_data["name"],
                sym_data["kind"],
                sym_data.get("signature"),
                sym_data.get("line_start"),
                sym_data.get("line_end"),
                sym_data.get("complexity", 0),
            ),
        )

    # Insérer les relations
    for rel_data in sample_relations:
        db.execute(
            """
            INSERT INTO relations (source_id, target_id, relation_type, location_line)
            VALUES (?, ?, ?, ?)
            """,
            (
                rel_data["source_id"],
                rel_data["target_id"],
                rel_data["relation_type"],
                rel_data.get("location_line"),
            ),
        )

    # Insérer les relations de fichiers
    for frel_data in sample_file_relations:
        db.execute(
            """
            INSERT INTO file_relations (source_file_id, target_file_id, relation_type, line_number)
            VALUES (?, ?, ?, ?)
            """,
            (
                frel_data["source_file_id"],
                frel_data["target_file_id"],
                frel_data["relation_type"],
                frel_data.get("line_number"),
            ),
        )

    # Insérer les erreurs
    for err_data in sample_errors:
        db.execute(
            """
            INSERT INTO error_history (file_path, symbol_name, error_type, severity, title,
                                       description, discovered_at, resolved_at, resolution, is_regression)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                err_data["file_path"],
                err_data.get("symbol_name"),
                err_data["error_type"],
                err_data["severity"],
                err_data["title"],
                err_data.get("description"),
                err_data["discovered_at"],
                err_data.get("resolved_at"),
                err_data.get("resolution"),
                err_data.get("is_regression", False),
            ),
        )

    # Insérer les patterns
    for pat_data in sample_patterns:
        db.execute(
            """
            INSERT INTO patterns (name, category, title, description, severity, scope, module,
                                  good_example, bad_example, is_active)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                pat_data["name"],
                pat_data["category"],
                pat_data["title"],
                pat_data["description"],
                pat_data["severity"],
                pat_data.get("scope", "project"),
                pat_data.get("module"),
                pat_data.get("good_example"),
                pat_data.get("bad_example"),
                pat_data["is_active"],
            ),
        )

    # Insérer les ADRs
    for adr_data in sample_adrs:
        db.execute(
            """
            INSERT INTO architecture_decisions (decision_id, title, status, context, decision,
                                                consequences, date_decided, decided_by, affected_modules_json)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                adr_data["decision_id"],
                adr_data["title"],
                adr_data["status"],
                adr_data["context"],
                adr_data["decision"],
                adr_data.get("consequences"),
                adr_data.get("date_decided"),
                adr_data.get("decided_by"),
                adr_data.get("affected_modules_json"),
            ),
        )

    db.commit()
    return db


# =============================================================================
# HELPERS
# =============================================================================

@pytest.fixture
def assert_performance():
    """Helper pour vérifier les performances."""
    import time

    class PerformanceChecker:
        def __init__(self, max_ms: float = 100.0):
            self.max_ms = max_ms
            self.start_time = None
            self.elapsed_ms = None

        def __enter__(self):
            self.start_time = time.perf_counter()
            return self

        def __exit__(self, *args):
            self.elapsed_ms = (time.perf_counter() - self.start_time) * 1000

        def check(self):
            assert self.elapsed_ms is not None, "Must use as context manager"
            assert self.elapsed_ms < self.max_ms, \
                f"Performance check failed: {self.elapsed_ms:.2f}ms > {self.max_ms}ms"

    return PerformanceChecker
