"""
Tests pour les modèles de données AgentDB.

Teste :
- Création des dataclasses
- Conversion to_dict()
- Création depuis une row SQLite (from_row)
- Les Enums
"""

import pytest
from datetime import datetime
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from agentdb.models import (
    File, Symbol, Relation, FileRelation,
    ErrorHistory, PipelineRun, Pattern, ArchitectureDecision, CriticalPath,
    SymbolKind, RelationType, Severity, ErrorType,
    CallerInfo, ImpactAnalysis,
)


# =============================================================================
# TESTS DES ENUMS
# =============================================================================

class TestEnums:
    """Tests pour les Enums."""

    def test_symbol_kind_values(self):
        """Vérifie les valeurs de SymbolKind."""
        assert SymbolKind.FUNCTION.value == "function"
        assert SymbolKind.STRUCT.value == "struct"
        assert SymbolKind.CLASS.value == "class"
        assert SymbolKind.ENUM.value == "enum"
        assert SymbolKind.MACRO.value == "macro"
        assert SymbolKind.VARIABLE.value == "variable"
        assert SymbolKind.TYPEDEF.value == "typedef"

    def test_relation_type_values(self):
        """Vérifie les valeurs de RelationType."""
        assert RelationType.CALLS.value == "calls"
        assert RelationType.USES_TYPE.value == "uses_type"
        assert RelationType.INCLUDES.value == "includes"
        assert RelationType.INHERITS.value == "inherits"

    def test_severity_values(self):
        """Vérifie les valeurs de Severity."""
        assert Severity.CRITICAL.value == "critical"
        assert Severity.HIGH.value == "high"
        assert Severity.MEDIUM.value == "medium"
        assert Severity.LOW.value == "low"

    def test_error_type_values(self):
        """Vérifie les valeurs de ErrorType."""
        assert ErrorType.MEMORY_LEAK.value == "memory_leak"
        assert ErrorType.NULL_POINTER.value == "null_pointer"
        assert ErrorType.BUFFER_OVERFLOW.value == "buffer_overflow"
        assert ErrorType.LOGIC_ERROR.value == "logic_error"

    def test_enum_from_string(self):
        """Teste la création d'enum depuis une chaîne."""
        assert SymbolKind("function") == SymbolKind.FUNCTION
        assert Severity("critical") == Severity.CRITICAL

    def test_enum_invalid_value(self):
        """Teste la gestion des valeurs invalides."""
        with pytest.raises(ValueError):
            SymbolKind("invalid_kind")


# =============================================================================
# TESTS DE FILE
# =============================================================================

class TestFile:
    """Tests pour le modèle File."""

    def test_file_creation_minimal(self):
        """Teste la création d'un File avec les champs minimaux."""
        f = File(path="src/main.c", filename="main.c")
        assert f.path == "src/main.c"
        assert f.filename == "main.c"
        assert f.id is None
        assert f.is_critical is False
        assert f.lines_total == 0

    def test_file_creation_full(self):
        """Teste la création d'un File avec tous les champs."""
        f = File(
            id=1,
            path="src/main.c",
            filename="main.c",
            extension=".c",
            module="core",
            layer="application",
            file_type="source",
            language="c",
            is_critical=True,
            criticality_reason="Entry point",
            security_sensitive=True,
            lines_total=100,
            lines_code=80,
            lines_comment=15,
            lines_blank=5,
            complexity_sum=50,
            complexity_avg=10.0,
            complexity_max=20,
            commits_30d=5,
            commits_90d=15,
            commits_365d=50,
            last_modified="2024-01-15",
        )
        assert f.id == 1
        assert f.is_critical is True
        assert f.complexity_avg == 10.0
        assert f.commits_30d == 5

    def test_file_to_dict(self):
        """Teste la conversion en dictionnaire."""
        f = File(
            id=1,
            path="src/main.c",
            filename="main.c",
            extension=".c",
            module="core",
            is_critical=True,
            lines_code=80,
        )
        d = f.to_dict()

        assert isinstance(d, dict)
        assert d["id"] == 1
        assert d["path"] == "src/main.c"
        assert d["module"] == "core"
        assert d["is_critical"] == 1  # to_dict returns int for SQLite compatibility
        assert d["lines_code"] == 80

    def test_file_from_row(self):
        """Teste la création depuis une row SQLite."""
        row = {
            "id": 1,
            "path": "src/main.c",
            "filename": "main.c",
            "extension": ".c",
            "module": "core",
            "is_critical": 1,
            "lines_code": 80,
            "complexity_avg": 10.5,
        }
        f = File.from_row(row)

        assert f.id == 1
        assert f.path == "src/main.c"
        assert f.module == "core"
        assert f.is_critical is True  # Converti de 1 à True
        assert f.lines_code == 80
        assert f.complexity_avg == 10.5

    def test_file_from_row_with_missing_fields(self):
        """Teste from_row avec des champs manquants."""
        row = {"id": 1, "path": "test.c", "filename": "test.c"}
        f = File.from_row(row)

        assert f.id == 1
        assert f.path == "test.c"
        assert f.module is None
        assert f.is_critical is False
        assert f.lines_code == 0


# =============================================================================
# TESTS DE SYMBOL
# =============================================================================

class TestSymbol:
    """Tests pour le modèle Symbol."""

    def test_symbol_creation_minimal(self):
        """Teste la création d'un Symbol minimal."""
        s = Symbol(file_id=1, name="main", kind="function")
        assert s.file_id == 1
        assert s.name == "main"
        assert s.kind == "function"
        assert s.id is None
        assert s.complexity == 0

    def test_symbol_creation_full(self):
        """Teste la création d'un Symbol complet."""
        s = Symbol(
            id=1,
            file_id=1,
            name="lcd_init",
            qualified_name="lcd::lcd_init",
            kind="function",
            line_start=10,
            line_end=50,
            signature="int lcd_init(LCD_Config *config)",
            return_type="int",
            visibility="public",
            is_exported=True,
            complexity=12,
            lines_of_code=40,
            doc_comment="Initialize the LCD display",
            has_doc=True,
        )
        assert s.id == 1
        assert s.qualified_name == "lcd::lcd_init"
        assert s.signature == "int lcd_init(LCD_Config *config)"
        assert s.complexity == 12
        assert s.has_doc is True

    def test_symbol_to_dict(self):
        """Teste la conversion en dictionnaire."""
        s = Symbol(
            id=1,
            file_id=1,
            name="main",
            kind="function",
            line_start=10,
            complexity=5,
        )
        d = s.to_dict()

        assert isinstance(d, dict)
        assert d["id"] == 1
        assert d["name"] == "main"
        assert d["kind"] == "function"
        assert d["complexity"] == 5

    def test_symbol_from_row(self):
        """Teste la création depuis une row SQLite."""
        row = {
            "id": 1,
            "file_id": 2,
            "name": "lcd_init",
            "kind": "function",
            "line_start": 10,
            "line_end": 50,
            "signature": "int lcd_init(void)",
            "complexity": 8,
            "is_exported": 1,
            "has_doc": 0,
        }
        s = Symbol.from_row(row)

        assert s.id == 1
        assert s.file_id == 2
        assert s.name == "lcd_init"
        assert s.is_exported is True
        assert s.has_doc is False
        assert s.complexity == 8


# =============================================================================
# TESTS DE RELATION
# =============================================================================

class TestRelation:
    """Tests pour le modèle Relation."""

    def test_relation_creation(self):
        """Teste la création d'une Relation."""
        r = Relation(
            source_id=1,
            target_id=2,
            relation_type="calls",
        )
        assert r.source_id == 1
        assert r.target_id == 2
        assert r.relation_type == "calls"
        assert r.is_direct is True
        assert r.count == 1

    def test_relation_with_location(self):
        """Teste une Relation avec localisation."""
        r = Relation(
            id=1,
            source_id=1,
            target_id=2,
            relation_type="calls",
            location_file_id=1,
            location_line=25,
            is_direct=True,
            count=3,
        )
        assert r.location_line == 25
        assert r.count == 3

    def test_relation_to_dict(self):
        """Teste la conversion en dictionnaire."""
        r = Relation(
            id=1,
            source_id=1,
            target_id=2,
            relation_type="calls",
            location_line=25,
        )
        d = r.to_dict()

        assert d["source_id"] == 1
        assert d["target_id"] == 2
        assert d["relation_type"] == "calls"

    def test_relation_from_row(self):
        """Teste la création depuis une row SQLite."""
        row = {
            "id": 1,
            "source_id": 10,
            "target_id": 20,
            "relation_type": "uses_type",
            "location_line": 30,
            "is_direct": 1,
            "count": 2,
        }
        r = Relation.from_row(row)

        assert r.source_id == 10
        assert r.target_id == 20
        assert r.relation_type == "uses_type"
        assert r.is_direct is True


# =============================================================================
# TESTS DE FILE_RELATION
# =============================================================================

class TestFileRelation:
    """Tests pour le modèle FileRelation."""

    def test_file_relation_creation(self):
        """Teste la création d'une FileRelation."""
        fr = FileRelation(
            source_file_id=1,
            target_file_id=2,
            relation_type="includes",
        )
        assert fr.source_file_id == 1
        assert fr.target_file_id == 2
        assert fr.relation_type == "includes"

    def test_file_relation_with_line(self):
        """Teste une FileRelation avec numéro de ligne."""
        fr = FileRelation(
            id=1,
            source_file_id=1,
            target_file_id=2,
            relation_type="includes",
            line_number=5,
            is_direct=True,
        )
        assert fr.line_number == 5
        assert fr.is_direct is True


# =============================================================================
# TESTS DE ERROR_HISTORY
# =============================================================================

class TestErrorHistory:
    """Tests pour le modèle ErrorHistory."""

    def test_error_history_creation(self):
        """Teste la création d'une ErrorHistory."""
        now = datetime.now().isoformat()
        e = ErrorHistory(
            file_path="src/main.c",
            error_type="memory_leak",
            severity="high",
            title="Memory leak in main",
            discovered_at=now,
        )
        assert e.file_path == "src/main.c"
        assert e.error_type == "memory_leak"
        assert e.severity == "high"
        assert e.is_regression is False

    def test_error_history_full(self):
        """Teste une ErrorHistory complète."""
        e = ErrorHistory(
            id=1,
            file_id=1,
            file_path="src/lcd/init.c",
            symbol_name="lcd_init",
            error_type="null_pointer",
            severity="critical",
            title="Null pointer dereference",
            description="Dereference of null pointer in error path",
            resolution="Added null check before dereference",
            prevention="Always validate pointers from external sources",
            discovered_at="2024-01-10",
            resolved_at="2024-01-12",
            is_regression=True,
        )
        assert e.symbol_name == "lcd_init"
        assert e.is_regression is True
        assert e.resolution is not None

    def test_error_history_to_dict(self):
        """Teste la conversion en dictionnaire."""
        e = ErrorHistory(
            id=1,
            file_path="src/main.c",
            error_type="logic_error",
            severity="medium",
            title="Off-by-one",
            discovered_at="2024-01-10",
        )
        d = e.to_dict()

        assert d["id"] == 1
        assert d["error_type"] == "logic_error"
        assert d["severity"] == "medium"


# =============================================================================
# TESTS DE PATTERN
# =============================================================================

class TestPattern:
    """Tests pour le modèle Pattern."""

    def test_pattern_creation(self):
        """Teste la création d'un Pattern."""
        p = Pattern(
            name="error_check",
            category="error_handling",
            title="Check Return Values",
            description="Always check return values",
        )
        assert p.name == "error_check"
        assert p.category == "error_handling"
        assert p.is_active is True
        assert p.severity == "warning"

    def test_pattern_full(self):
        """Teste un Pattern complet."""
        p = Pattern(
            id=1,
            name="lcd_naming",
            category="naming_convention",
            scope="module",
            module="lcd",
            title="LCD Naming Convention",
            description="LCD functions must start with lcd_",
            rationale="Consistency and discoverability",
            good_example="lcd_init(), lcd_write()",
            bad_example="init_lcd(), write_to_lcd()",
            severity="high",
            is_active=True,
        )
        assert p.scope == "module"
        assert p.module == "lcd"
        assert p.good_example is not None

    def test_pattern_to_dict(self):
        """Teste la conversion en dictionnaire."""
        p = Pattern(
            id=1,
            name="test_pattern",
            category="testing",
            title="Test Pattern",
            description="A test pattern",
        )
        d = p.to_dict()

        assert d["name"] == "test_pattern"
        assert d["category"] == "testing"


# =============================================================================
# TESTS DE ARCHITECTURE_DECISION
# =============================================================================

class TestArchitectureDecision:
    """Tests pour le modèle ArchitectureDecision."""

    def test_adr_creation(self):
        """Teste la création d'un ADR."""
        adr = ArchitectureDecision(
            decision_id="ADR-001",
            title="Use SQLite",
            status="accepted",
            context="Need local database",
            decision="Use SQLite with WAL mode",
        )
        assert adr.decision_id == "ADR-001"
        assert adr.status == "accepted"

    def test_adr_full(self):
        """Teste un ADR complet."""
        adr = ArchitectureDecision(
            id=1,
            decision_id="ADR-002",
            title="Error Handling Strategy",
            status="accepted",
            context="Need consistent error handling",
            decision="Use return codes for C",
            consequences="Clear error propagation",
            alternatives="Exceptions, setjmp/longjmp",
            date_proposed="2024-01-01",
            date_decided="2024-01-15",
            decided_by="Tech Lead",
        )
        assert adr.consequences is not None
        assert adr.alternatives is not None

    def test_adr_to_dict(self):
        """Teste la conversion en dictionnaire."""
        adr = ArchitectureDecision(
            decision_id="ADR-001",
            title="Test ADR",
            status="proposed",
            context="Context",
            decision="Decision",
        )
        d = adr.to_dict()

        assert d["decision_id"] == "ADR-001"
        assert d["status"] == "proposed"


# =============================================================================
# TESTS DE CRITICAL_PATH
# =============================================================================

class TestCriticalPath:
    """Tests pour le modèle CriticalPath."""

    def test_critical_path_creation(self):
        """Teste la création d'un CriticalPath."""
        cp = CriticalPath(
            pattern="**/security/**",
            reason="Security-sensitive code",
        )
        assert cp.pattern == "**/security/**"
        assert cp.severity == "high"

    def test_critical_path_full(self):
        """Teste un CriticalPath complet."""
        cp = CriticalPath(
            id=1,
            pattern="**/main.*",
            reason="Application entry point",
            severity="critical",
            added_by="system",
        )
        assert cp.severity == "critical"
        assert cp.added_by == "system"


# =============================================================================
# TESTS DES DATACLASSES UTILITAIRES
# =============================================================================

class TestCallerInfo:
    """Tests pour CallerInfo."""

    def test_caller_info_creation(self):
        """Teste la création d'un CallerInfo."""
        ci = CallerInfo(
            id=1,
            name="main",
            kind="function",
            file_path="src/main.c",
            line=25,
            depth=1,
        )
        assert ci.name == "main"
        assert ci.depth == 1
        assert ci.is_direct is True
        assert ci.is_critical is False

    def test_caller_info_from_row(self):
        """Teste la création depuis une row."""
        row = {
            "id": 1,
            "name": "main",
            "kind": "function",
            "file_path": "src/main.c",
            "location_line": 25,
            "depth": 2,
            "is_direct": 1,
            "is_critical": 0,
        }
        ci = CallerInfo.from_row(row)
        assert ci.name == "main"
        assert ci.line == 25
        assert ci.depth == 2


class TestImpactAnalysis:
    """Tests pour ImpactAnalysis."""

    def test_impact_analysis_creation(self):
        """Teste la création d'un ImpactAnalysis."""
        ia = ImpactAnalysis(
            file_path="src/main.c",
        )
        assert ia.file_path == "src/main.c"
        assert ia.direct_impact == []
        assert ia.transitive_impact == []
        assert ia.include_impact == []

    def test_impact_analysis_with_impacts(self):
        """Teste un ImpactAnalysis avec impacts."""
        ia = ImpactAnalysis(
            file_path="src/main.c",
            direct_impact=[{"file": "src/other.c", "reason": "calls main"}],
            transitive_impact=[{"file": "src/deep.c", "depth": 2}],
            summary={"total_files_impacted": 2},
        )
        assert len(ia.direct_impact) == 1
        assert ia.total_impact_count == 2

    def test_impact_analysis_to_dict(self):
        """Teste la conversion en dictionnaire."""
        ia = ImpactAnalysis(
            file_path="src/main.c",
            direct_impact=[{"file": "src/other.c"}],
        )
        d = ia.to_dict()
        assert d["file_path"] == "src/main.c"
        assert len(d["direct_impact"]) == 1


# =============================================================================
# TESTS D'ÉGALITÉ ET HASH
# =============================================================================

class TestEquality:
    """Tests pour l'égalité et le hash des dataclasses."""

    def test_file_equality(self):
        """Teste l'égalité de File."""
        f1 = File(id=1, path="src/main.c", filename="main.c")
        f2 = File(id=1, path="src/main.c", filename="main.c")
        f3 = File(id=2, path="src/main.c", filename="main.c")

        assert f1 == f2
        assert f1 != f3

    def test_symbol_equality(self):
        """Teste l'égalité de Symbol."""
        s1 = Symbol(id=1, file_id=1, name="main", kind="function")
        s2 = Symbol(id=1, file_id=1, name="main", kind="function")
        s3 = Symbol(id=2, file_id=1, name="main", kind="function")

        assert s1 == s2
        assert s1 != s3

    def test_relation_equality(self):
        """Teste l'égalité de Relation."""
        r1 = Relation(id=1, source_id=1, target_id=2, relation_type="calls")
        r2 = Relation(id=1, source_id=1, target_id=2, relation_type="calls")

        assert r1 == r2
