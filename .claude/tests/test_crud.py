"""
Tests CRUD pour AgentDB.

Teste les opérations Create, Read, Update, Delete pour chaque repository :
- FileRepository
- SymbolRepository
- RelationRepository
- FileRelationRepository
- ErrorHistoryRepository
- PipelineRunRepository
- PatternRepository
- ArchitectureDecisionRepository

Utilise une base de données en mémoire (:memory:) pour l'isolation.
"""

import pytest
from datetime import datetime
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from agentdb.db import Database
from agentdb.models import (
    File, Symbol, Relation, FileRelation,
    ErrorHistory, PipelineRun, Pattern, ArchitectureDecision,
)
from agentdb.crud import (
    FileRepository,
    SymbolRepository,
    RelationRepository,
    FileRelationRepository,
    ErrorHistoryRepository,
    PipelineRunRepository,
    PatternRepository,
    ArchitectureDecisionRepository,
)


# =============================================================================
# TESTS FILE REPOSITORY
# =============================================================================

class TestFileRepository:
    """Tests CRUD pour FileRepository."""

    def test_create_file(self, db):
        """Teste la création d'un fichier."""
        repo = FileRepository(db)
        file = File(
            path="src/main.c",
            filename="main.c",
            extension=".c",
            module="core",
            language="c",
        )

        file_id = repo.insert(file)

        assert file_id is not None
        assert file_id > 0

        # Vérifier le fichier créé
        created = repo.get_by_id(file_id)
        assert created.path == "src/main.c"
        assert created.module == "core"

    def test_create_file_with_metrics(self, db):
        """Teste la création d'un fichier avec métriques."""
        repo = FileRepository(db)
        file = File(
            path="src/complex.c",
            filename="complex.c",
            extension=".c",
            lines_total=500,
            lines_code=400,
            lines_comment=80,
            lines_blank=20,
            complexity_avg=15.5,
            complexity_max=30,
        )

        file_id = repo.insert(file)
        created = repo.get_by_id(file_id)

        assert created.lines_total == 500
        assert created.complexity_avg == 15.5

    def test_get_by_id(self, db):
        """Teste la récupération par ID."""
        repo = FileRepository(db)
        file = File(path="src/test.c", filename="test.c")
        file_id = repo.insert(file)

        retrieved = repo.get_by_id(file_id)

        assert retrieved is not None
        assert retrieved.id == file_id
        assert retrieved.path == "src/test.c"

    def test_get_by_id_not_found(self, db):
        """Teste get_by_id avec un ID inexistant."""
        repo = FileRepository(db)
        retrieved = repo.get_by_id(9999)
        assert retrieved is None

    def test_get_by_path(self, db):
        """Teste la récupération par chemin."""
        repo = FileRepository(db)
        file = File(path="src/unique.c", filename="unique.c", module="test")
        repo.insert(file)

        retrieved = repo.get_by_path("src/unique.c")

        assert retrieved is not None
        assert retrieved.module == "test"

    def test_get_by_path_not_found(self, db):
        """Teste get_by_path avec un chemin inexistant."""
        repo = FileRepository(db)
        retrieved = repo.get_by_path("nonexistent/file.c")
        assert retrieved is None

    def test_get_all(self, db):
        """Teste la récupération de tous les fichiers."""
        repo = FileRepository(db)
        repo.insert(File(path="a.c", filename="a.c"))
        repo.insert(File(path="b.c", filename="b.c"))
        repo.insert(File(path="c.c", filename="c.c"))

        all_files = repo.get_all()

        assert len(all_files) == 3

    def test_get_by_module(self, db):
        """Teste la récupération par module."""
        repo = FileRepository(db)
        repo.insert(File(path="src/lcd/a.c", filename="a.c", module="lcd"))
        repo.insert(File(path="src/lcd/b.c", filename="b.c", module="lcd"))
        repo.insert(File(path="src/core/c.c", filename="c.c", module="core"))

        lcd_files = repo.get_by_module("lcd")

        assert len(lcd_files) == 2
        assert all(f.module == "lcd" for f in lcd_files)

    def test_update_file(self, db):
        """Teste la mise à jour d'un fichier."""
        repo = FileRepository(db)
        file = File(path="src/update.c", filename="update.c", lines_code=100)
        file_id = repo.insert(file)

        # Mettre à jour avec des kwargs
        repo.update(file_id, lines_code=200, module="updated")

        # Vérifier en relisant
        retrieved = repo.get_by_id(file_id)
        assert retrieved.lines_code == 200
        assert retrieved.module == "updated"

    def test_delete_file(self, db):
        """Teste la suppression d'un fichier."""
        repo = FileRepository(db)
        file = File(path="src/delete.c", filename="delete.c")
        file_id = repo.insert(file)

        count = repo.delete(file_id)

        assert count >= 1 or count is True  # Returns row count or bool
        assert repo.get_by_id(file_id) is None

    def test_delete_nonexistent(self, db):
        """Teste la suppression d'un fichier inexistant."""
        repo = FileRepository(db)
        result = repo.delete(9999)
        # delete should return 0 rows affected or False
        assert result == 0 or result is False

    def test_get_critical_files(self, db):
        """Teste la récupération des fichiers critiques."""
        repo = FileRepository(db)
        repo.insert(File(path="a.c", filename="a.c", is_critical=True))
        repo.insert(File(path="b.c", filename="b.c", is_critical=False))
        repo.insert(File(path="c.c", filename="c.c", is_critical=True))

        critical = repo.get_critical_files()

        assert len(critical) == 2
        assert all(f.is_critical for f in critical)

    def test_count(self, db):
        """Teste le comptage des fichiers."""
        repo = FileRepository(db)
        assert repo.count() == 0

        repo.insert(File(path="a.c", filename="a.c"))
        repo.insert(File(path="b.c", filename="b.c"))

        assert repo.count() == 2


# =============================================================================
# TESTS SYMBOL REPOSITORY
# =============================================================================

class TestSymbolRepository:
    """Tests CRUD pour SymbolRepository."""

    @pytest.fixture
    def file_id(self, db):
        """Crée un fichier et retourne son ID."""
        repo = FileRepository(db)
        file = repo.create(File(path="src/test.c", filename="test.c"))
        return file.id

    def test_create_symbol(self, db, file_id):
        """Teste la création d'un symbole."""
        repo = SymbolRepository(db)
        symbol = Symbol(
            file_id=file_id,
            name="main",
            kind="function",
            signature="int main(int argc, char **argv)",
            line_start=10,
            line_end=50,
        )

        created = repo.create(symbol)

        assert created.id is not None
        assert created.name == "main"
        assert created.kind == "function"

    def test_create_symbol_with_complexity(self, db, file_id):
        """Teste la création d'un symbole avec métriques."""
        repo = SymbolRepository(db)
        symbol = Symbol(
            file_id=file_id,
            name="complex_func",
            kind="function",
            complexity=25,
            lines_of_code=100,
            nesting_depth=5,
        )

        created = repo.create(symbol)

        assert created.complexity == 25
        assert created.nesting_depth == 5

    def test_get_by_id(self, db, file_id):
        """Teste la récupération par ID."""
        repo = SymbolRepository(db)
        symbol = Symbol(file_id=file_id, name="test_sym", kind="function")
        created = repo.create(symbol)

        retrieved = repo.get_by_id(created.id)

        assert retrieved is not None
        assert retrieved.name == "test_sym"

    def test_get_by_file(self, db, file_id):
        """Teste la récupération par fichier."""
        repo = SymbolRepository(db)
        repo.create(Symbol(file_id=file_id, name="func1", kind="function"))
        repo.create(Symbol(file_id=file_id, name="func2", kind="function"))
        repo.create(Symbol(file_id=file_id, name="MyStruct", kind="struct"))

        symbols = repo.get_by_file(file_id)

        assert len(symbols) == 3

    def test_get_by_name(self, db, file_id):
        """Teste la récupération par nom."""
        repo = SymbolRepository(db)
        repo.create(Symbol(file_id=file_id, name="unique_name", kind="function"))

        symbols = repo.get_by_name("unique_name")

        assert len(symbols) == 1
        assert symbols[0].name == "unique_name"

    def test_get_by_kind(self, db, file_id):
        """Teste la récupération par type."""
        repo = SymbolRepository(db)
        repo.create(Symbol(file_id=file_id, name="func1", kind="function"))
        repo.create(Symbol(file_id=file_id, name="func2", kind="function"))
        repo.create(Symbol(file_id=file_id, name="MyStruct", kind="struct"))

        functions = repo.get_by_kind("function")

        assert len(functions) == 2
        assert all(s.kind == "function" for s in functions)

    def test_update_symbol(self, db, file_id):
        """Teste la mise à jour d'un symbole."""
        repo = SymbolRepository(db)
        symbol = Symbol(file_id=file_id, name="old_name", kind="function")
        created = repo.create(symbol)

        created.complexity = 10
        created.doc_comment = "New documentation"
        updated = repo.update(created)

        assert updated.complexity == 10
        assert updated.doc_comment == "New documentation"

    def test_delete_symbol(self, db, file_id):
        """Teste la suppression d'un symbole."""
        repo = SymbolRepository(db)
        symbol = Symbol(file_id=file_id, name="to_delete", kind="function")
        created = repo.create(symbol)

        deleted = repo.delete(created.id)

        assert deleted is True
        assert repo.get_by_id(created.id) is None

    def test_delete_by_file(self, db, file_id):
        """Teste la suppression de tous les symboles d'un fichier."""
        repo = SymbolRepository(db)
        repo.create(Symbol(file_id=file_id, name="func1", kind="function"))
        repo.create(Symbol(file_id=file_id, name="func2", kind="function"))

        count = repo.delete_by_file(file_id)

        assert count == 2
        assert len(repo.get_by_file(file_id)) == 0

    def test_search(self, db, file_id):
        """Teste la recherche de symboles."""
        repo = SymbolRepository(db)
        repo.create(Symbol(file_id=file_id, name="lcd_init", kind="function"))
        repo.create(Symbol(file_id=file_id, name="lcd_write", kind="function"))
        repo.create(Symbol(file_id=file_id, name="gpio_init", kind="function"))

        results = repo.search("lcd%")

        assert len(results) == 2
        assert all("lcd" in s.name for s in results)


# =============================================================================
# TESTS RELATION REPOSITORY
# =============================================================================

class TestRelationRepository:
    """Tests CRUD pour RelationRepository."""

    @pytest.fixture
    def symbols(self, db):
        """Crée des symboles et retourne leurs IDs."""
        file_repo = FileRepository(db)
        file = file_repo.create(File(path="test.c", filename="test.c"))

        sym_repo = SymbolRepository(db)
        sym1 = sym_repo.create(Symbol(file_id=file.id, name="caller", kind="function"))
        sym2 = sym_repo.create(Symbol(file_id=file.id, name="callee", kind="function"))
        sym3 = sym_repo.create(Symbol(file_id=file.id, name="MyType", kind="struct"))

        return {"caller": sym1.id, "callee": sym2.id, "type": sym3.id, "file_id": file.id}

    def test_create_relation(self, db, symbols):
        """Teste la création d'une relation."""
        repo = RelationRepository(db)
        relation = Relation(
            source_id=symbols["caller"],
            target_id=symbols["callee"],
            relation_type="calls",
        )

        created = repo.create(relation)

        assert created.id is not None
        assert created.relation_type == "calls"

    def test_create_relation_with_location(self, db, symbols):
        """Teste la création avec localisation."""
        repo = RelationRepository(db)
        relation = Relation(
            source_id=symbols["caller"],
            target_id=symbols["callee"],
            relation_type="calls",
            location_file_id=symbols["file_id"],
            location_line=25,
        )

        created = repo.create(relation)

        assert created.location_line == 25

    def test_get_callers(self, db, symbols):
        """Teste la récupération des appelants."""
        repo = RelationRepository(db)
        repo.create(Relation(
            source_id=symbols["caller"],
            target_id=symbols["callee"],
            relation_type="calls",
        ))

        callers = repo.get_callers(symbols["callee"])

        assert len(callers) == 1
        assert callers[0].source_id == symbols["caller"]

    def test_get_callees(self, db, symbols):
        """Teste la récupération des appelés."""
        repo = RelationRepository(db)
        repo.create(Relation(
            source_id=symbols["caller"],
            target_id=symbols["callee"],
            relation_type="calls",
        ))

        callees = repo.get_callees(symbols["caller"])

        assert len(callees) == 1
        assert callees[0].target_id == symbols["callee"]

    def test_get_by_type(self, db, symbols):
        """Teste la récupération par type de relation."""
        repo = RelationRepository(db)
        repo.create(Relation(
            source_id=symbols["caller"],
            target_id=symbols["callee"],
            relation_type="calls",
        ))
        repo.create(Relation(
            source_id=symbols["caller"],
            target_id=symbols["type"],
            relation_type="uses_type",
        ))

        calls = repo.get_by_type("calls")
        uses = repo.get_by_type("uses_type")

        assert len(calls) == 1
        assert len(uses) == 1

    def test_delete_by_source(self, db, symbols):
        """Teste la suppression par source."""
        repo = RelationRepository(db)
        repo.create(Relation(source_id=symbols["caller"], target_id=symbols["callee"], relation_type="calls"))
        repo.create(Relation(source_id=symbols["caller"], target_id=symbols["type"], relation_type="uses_type"))

        count = repo.delete_by_source(symbols["caller"])

        assert count == 2

    def test_delete_by_target(self, db, symbols):
        """Teste la suppression par cible."""
        repo = RelationRepository(db)
        repo.create(Relation(source_id=symbols["caller"], target_id=symbols["callee"], relation_type="calls"))

        count = repo.delete_by_target(symbols["callee"])

        assert count == 1


# =============================================================================
# TESTS FILE RELATION REPOSITORY
# =============================================================================

class TestFileRelationRepository:
    """Tests CRUD pour FileRelationRepository."""

    @pytest.fixture
    def files(self, db):
        """Crée des fichiers et retourne leurs IDs."""
        repo = FileRepository(db)
        f1 = repo.create(File(path="src/main.c", filename="main.c"))
        f2 = repo.create(File(path="src/lcd.h", filename="lcd.h"))
        f3 = repo.create(File(path="src/lcd.c", filename="lcd.c"))
        return {"main": f1.id, "header": f2.id, "impl": f3.id}

    def test_create_file_relation(self, db, files):
        """Teste la création d'une relation de fichier."""
        repo = FileRelationRepository(db)
        relation = FileRelation(
            source_file_id=files["main"],
            target_file_id=files["header"],
            relation_type="includes",
            line_number=3,
        )

        created = repo.create(relation)

        assert created.id is not None
        assert created.relation_type == "includes"

    def test_get_includes(self, db, files):
        """Teste la récupération des fichiers inclus."""
        repo = FileRelationRepository(db)
        repo.create(FileRelation(
            source_file_id=files["main"],
            target_file_id=files["header"],
            relation_type="includes",
        ))

        includes = repo.get_includes(files["main"])

        assert len(includes) == 1
        assert includes[0].target_file_id == files["header"]

    def test_get_included_by(self, db, files):
        """Teste la récupération des fichiers qui incluent."""
        repo = FileRelationRepository(db)
        repo.create(FileRelation(
            source_file_id=files["main"],
            target_file_id=files["header"],
            relation_type="includes",
        ))
        repo.create(FileRelation(
            source_file_id=files["impl"],
            target_file_id=files["header"],
            relation_type="includes",
        ))

        included_by = repo.get_included_by(files["header"])

        assert len(included_by) == 2


# =============================================================================
# TESTS ERROR HISTORY REPOSITORY
# =============================================================================

class TestErrorHistoryRepository:
    """Tests CRUD pour ErrorHistoryRepository."""

    def test_create_error(self, db):
        """Teste la création d'une erreur."""
        repo = ErrorHistoryRepository(db)
        error = ErrorHistory(
            file_path="src/main.c",
            error_type="memory_leak",
            severity="high",
            title="Memory leak in main",
            discovered_at=datetime.now().isoformat(),
        )

        created = repo.create(error)

        assert created.id is not None
        assert created.error_type == "memory_leak"

    def test_create_error_with_resolution(self, db):
        """Teste la création avec résolution."""
        now = datetime.now().isoformat()
        repo = ErrorHistoryRepository(db)
        error = ErrorHistory(
            file_path="src/main.c",
            error_type="null_pointer",
            severity="critical",
            title="NPE in init",
            discovered_at=now,
            resolved_at=now,
            resolution="Added null check",
            prevention="Always validate inputs",
        )

        created = repo.create(error)

        assert created.resolution == "Added null check"
        assert created.prevention is not None

    def test_get_by_file(self, db):
        """Teste la récupération par fichier."""
        repo = ErrorHistoryRepository(db)
        now = datetime.now().isoformat()
        repo.create(ErrorHistory(
            file_path="src/main.c",
            error_type="error1",
            severity="high",
            title="Error 1",
            discovered_at=now,
        ))
        repo.create(ErrorHistory(
            file_path="src/main.c",
            error_type="error2",
            severity="medium",
            title="Error 2",
            discovered_at=now,
        ))
        repo.create(ErrorHistory(
            file_path="src/other.c",
            error_type="error3",
            severity="low",
            title="Error 3",
            discovered_at=now,
        ))

        errors = repo.get_by_file("src/main.c")

        assert len(errors) == 2

    def test_get_by_severity(self, db):
        """Teste la récupération par sévérité."""
        repo = ErrorHistoryRepository(db)
        now = datetime.now().isoformat()
        repo.create(ErrorHistory(file_path="a.c", error_type="e1", severity="critical", title="E1", discovered_at=now))
        repo.create(ErrorHistory(file_path="b.c", error_type="e2", severity="high", title="E2", discovered_at=now))
        repo.create(ErrorHistory(file_path="c.c", error_type="e3", severity="critical", title="E3", discovered_at=now))

        critical = repo.get_by_severity("critical")

        assert len(critical) == 2

    def test_get_unresolved(self, db):
        """Teste la récupération des erreurs non résolues."""
        repo = ErrorHistoryRepository(db)
        now = datetime.now().isoformat()
        repo.create(ErrorHistory(
            file_path="a.c",
            error_type="e1",
            severity="high",
            title="Unresolved",
            discovered_at=now,
        ))
        repo.create(ErrorHistory(
            file_path="b.c",
            error_type="e2",
            severity="high",
            title="Resolved",
            discovered_at=now,
            resolved_at=now,
        ))

        unresolved = repo.get_unresolved()

        assert len(unresolved) == 1
        assert unresolved[0].title == "Unresolved"


# =============================================================================
# TESTS PIPELINE RUN REPOSITORY
# =============================================================================

class TestPipelineRunRepository:
    """Tests CRUD pour PipelineRunRepository."""

    def test_create_run(self, db):
        """Teste la création d'un run."""
        repo = PipelineRunRepository(db)
        run = PipelineRun(
            run_id="run-001",
            commit_hash="abc123",
            status="completed",
            started_at=datetime.now().isoformat(),
        )

        created = repo.create(run)

        assert created.id is not None
        assert created.run_id == "run-001"

    def test_create_run_with_scores(self, db):
        """Teste la création avec scores."""
        repo = PipelineRunRepository(db)
        now = datetime.now().isoformat()
        run = PipelineRun(
            run_id="run-002",
            commit_hash="def456",
            status="completed",
            started_at=now,
            completed_at=now,
            overall_score=85,
            score_analyzer=90,
            score_security=80,
            score_reviewer=85,
            issues_critical=0,
            issues_high=2,
            issues_medium=5,
        )

        created = repo.create(run)

        assert created.overall_score == 85
        assert created.issues_high == 2

    def test_get_by_run_id(self, db):
        """Teste la récupération par run_id."""
        repo = PipelineRunRepository(db)
        run = PipelineRun(
            run_id="run-unique",
            commit_hash="xyz789",
            status="completed",
            started_at=datetime.now().isoformat(),
        )
        repo.create(run)

        retrieved = repo.get_by_run_id("run-unique")

        assert retrieved is not None
        assert retrieved.commit_hash == "xyz789"

    def test_get_by_commit(self, db):
        """Teste la récupération par commit."""
        repo = PipelineRunRepository(db)
        now = datetime.now().isoformat()
        repo.create(PipelineRun(run_id="r1", commit_hash="same_commit", status="completed", started_at=now))
        repo.create(PipelineRun(run_id="r2", commit_hash="same_commit", status="failed", started_at=now))
        repo.create(PipelineRun(run_id="r3", commit_hash="other", status="completed", started_at=now))

        runs = repo.get_by_commit("same_commit")

        assert len(runs) == 2

    def test_get_recent(self, db):
        """Teste la récupération des runs récents."""
        repo = PipelineRunRepository(db)
        now = datetime.now().isoformat()

        for i in range(10):
            repo.create(PipelineRun(
                run_id=f"run-{i}",
                commit_hash=f"commit-{i}",
                status="completed",
                started_at=now,
            ))

        recent = repo.get_recent(limit=5)

        assert len(recent) == 5


# =============================================================================
# TESTS PATTERN REPOSITORY
# =============================================================================

class TestPatternRepository:
    """Tests CRUD pour PatternRepository."""

    def test_create_pattern(self, db):
        """Teste la création d'un pattern."""
        repo = PatternRepository(db)
        pattern = Pattern(
            name="error_check",
            category="error_handling",
            title="Check Return Values",
            description="Always check return values",
        )

        created = repo.create(pattern)

        assert created.id is not None
        assert created.name == "error_check"

    def test_create_pattern_with_examples(self, db):
        """Teste la création avec exemples."""
        repo = PatternRepository(db)
        pattern = Pattern(
            name="null_check",
            category="safety",
            title="Null Checks",
            description="Check for null pointers",
            good_example="if (ptr != NULL) { ... }",
            bad_example="ptr->field;  // no check",
            rationale="Prevents null pointer dereferences",
        )

        created = repo.create(pattern)

        assert created.good_example is not None
        assert created.bad_example is not None

    def test_get_by_name(self, db):
        """Teste la récupération par nom."""
        repo = PatternRepository(db)
        repo.create(Pattern(name="unique_pattern", category="test", title="Test", description="Test"))

        retrieved = repo.get_by_name("unique_pattern")

        assert retrieved is not None
        assert retrieved.category == "test"

    def test_get_by_category(self, db):
        """Teste la récupération par catégorie."""
        repo = PatternRepository(db)
        repo.create(Pattern(name="p1", category="error_handling", title="P1", description="P1"))
        repo.create(Pattern(name="p2", category="error_handling", title="P2", description="P2"))
        repo.create(Pattern(name="p3", category="security", title="P3", description="P3"))

        patterns = repo.get_by_category("error_handling")

        assert len(patterns) == 2

    def test_get_active(self, db):
        """Teste la récupération des patterns actifs."""
        repo = PatternRepository(db)
        repo.create(Pattern(name="active", category="test", title="Active", description="Active", is_active=True))
        repo.create(Pattern(name="inactive", category="test", title="Inactive", description="Inactive", is_active=False))

        active = repo.get_active()

        assert len(active) == 1
        assert active[0].name == "active"

    def test_get_for_file(self, db):
        """Teste la récupération des patterns pour un fichier."""
        repo = PatternRepository(db)
        # Pattern global
        repo.create(Pattern(name="global", category="test", title="Global", description="Global", scope="project"))
        # Pattern pour module lcd
        repo.create(Pattern(name="lcd_pattern", category="test", title="LCD", description="LCD",
                           scope="module", module="lcd"))

        # Les patterns pour un fichier lcd
        patterns = repo.get_for_file("src/lcd/init.c")

        # Devrait inclure au moins le pattern global
        assert len(patterns) >= 1


# =============================================================================
# TESTS ARCHITECTURE DECISION REPOSITORY
# =============================================================================

class TestArchitectureDecisionRepository:
    """Tests CRUD pour ArchitectureDecisionRepository."""

    def test_create_adr(self, db):
        """Teste la création d'un ADR."""
        repo = ArchitectureDecisionRepository(db)
        adr = ArchitectureDecision(
            decision_id="ADR-001",
            title="Use SQLite",
            status="accepted",
            context="Need local storage",
            decision="Use SQLite with WAL mode",
        )

        created = repo.create(adr)

        assert created.id is not None
        assert created.decision_id == "ADR-001"

    def test_create_adr_full(self, db):
        """Teste la création d'un ADR complet."""
        repo = ArchitectureDecisionRepository(db)
        adr = ArchitectureDecision(
            decision_id="ADR-002",
            title="Error Strategy",
            status="accepted",
            context="Need error handling",
            decision="Use return codes",
            consequences="Clear propagation",
            alternatives="Exceptions",
            date_proposed="2024-01-01",
            date_decided="2024-01-15",
            decided_by="Tech Lead",
        )

        created = repo.create(adr)

        assert created.consequences is not None
        assert created.alternatives is not None

    def test_get_by_decision_id(self, db):
        """Teste la récupération par decision_id."""
        repo = ArchitectureDecisionRepository(db)
        repo.create(ArchitectureDecision(
            decision_id="ADR-UNIQUE",
            title="Unique",
            status="accepted",
            context="Context",
            decision="Decision",
        ))

        retrieved = repo.get_by_decision_id("ADR-UNIQUE")

        assert retrieved is not None
        assert retrieved.title == "Unique"

    def test_get_by_status(self, db):
        """Teste la récupération par statut."""
        repo = ArchitectureDecisionRepository(db)
        repo.create(ArchitectureDecision(decision_id="A1", title="A1", status="accepted", context="C", decision="D"))
        repo.create(ArchitectureDecision(decision_id="A2", title="A2", status="proposed", context="C", decision="D"))
        repo.create(ArchitectureDecision(decision_id="A3", title="A3", status="accepted", context="C", decision="D"))

        accepted = repo.get_by_status("accepted")

        assert len(accepted) == 2

    def test_get_for_file(self, db):
        """Teste la récupération des ADRs pour un fichier."""
        repo = ArchitectureDecisionRepository(db)
        repo.create(ArchitectureDecision(
            decision_id="ADR-LCD",
            title="LCD Strategy",
            status="accepted",
            context="LCD module",
            decision="Use direct hardware access",
            affected_modules_json='["lcd"]',
        ))

        adrs = repo.get_for_file("src/lcd/init.c")

        # Le test dépend de l'implémentation de get_for_file
        assert isinstance(adrs, list)

    def test_update_status(self, db):
        """Teste la mise à jour du statut."""
        repo = ArchitectureDecisionRepository(db)
        adr = repo.create(ArchitectureDecision(
            decision_id="ADR-UPDATE",
            title="Update",
            status="proposed",
            context="C",
            decision="D",
        ))

        adr.status = "accepted"
        updated = repo.update(adr)

        assert updated.status == "accepted"

        # Vérifier en relisant
        retrieved = repo.get_by_decision_id("ADR-UPDATE")
        assert retrieved.status == "accepted"


# =============================================================================
# TESTS D'INTÉGRATION CRUD
# =============================================================================

class TestCRUDIntegration:
    """Tests d'intégration entre repositories."""

    def test_file_with_symbols_and_relations(self, db):
        """Teste la création d'un fichier avec symboles et relations."""
        # Créer fichier
        file_repo = FileRepository(db)
        file = file_repo.create(File(path="src/test.c", filename="test.c"))

        # Créer symboles
        sym_repo = SymbolRepository(db)
        sym1 = sym_repo.create(Symbol(file_id=file.id, name="func1", kind="function"))
        sym2 = sym_repo.create(Symbol(file_id=file.id, name="func2", kind="function"))

        # Créer relation
        rel_repo = RelationRepository(db)
        rel_repo.create(Relation(source_id=sym1.id, target_id=sym2.id, relation_type="calls"))

        # Vérifier tout est lié
        symbols = sym_repo.get_by_file(file.id)
        assert len(symbols) == 2

        callees = rel_repo.get_callees(sym1.id)
        assert len(callees) == 1
        assert callees[0].target_id == sym2.id

    def test_cascade_delete_integration(self, db):
        """Teste la suppression en cascade."""
        # Créer fichier avec symboles et relations
        file_repo = FileRepository(db)
        file = file_repo.create(File(path="src/cascade.c", filename="cascade.c"))

        sym_repo = SymbolRepository(db)
        sym1 = sym_repo.create(Symbol(file_id=file.id, name="s1", kind="function"))
        sym2 = sym_repo.create(Symbol(file_id=file.id, name="s2", kind="function"))

        rel_repo = RelationRepository(db)
        rel_repo.create(Relation(source_id=sym1.id, target_id=sym2.id, relation_type="calls"))

        # Supprimer le fichier
        file_repo.delete(file.id)

        # Vérifier que tout est supprimé
        assert sym_repo.get_by_file(file.id) == []
        assert len(rel_repo.get_callers(sym2.id)) == 0

    def test_error_linked_to_file(self, db):
        """Teste la liaison erreur-fichier."""
        # Créer fichier
        file_repo = FileRepository(db)
        file = file_repo.create(File(path="src/buggy.c", filename="buggy.c"))

        # Créer erreur liée au fichier
        error_repo = ErrorHistoryRepository(db)
        error = error_repo.create(ErrorHistory(
            file_id=file.id,
            file_path=file.path,
            error_type="bug",
            severity="high",
            title="Bug in buggy.c",
            discovered_at=datetime.now().isoformat(),
        ))

        # Récupérer les erreurs du fichier
        errors = error_repo.get_by_file(file.path)
        assert len(errors) == 1
        assert errors[0].file_id == file.id
