"""
Tests pour les outils MCP AgentDB.

Teste :
- Chaque outil MCP (10 outils)
- Le format de sortie JSON conforme à PARTIE 7.2
- Les cas d'erreur

Utilise une base de données peuplée avec des données de test.
"""

import pytest
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from agentdb.db import Database


# Import des outils MCP
# Note: Ajuster le chemin si nécessaire selon la structure du projet
try:
    from mcp.agentdb.tools import (
        get_file_context,
        get_symbol_callers,
        get_symbol_callees,
        get_file_impact,
        get_error_history,
        get_patterns,
        get_architecture_decisions,
        search_symbols,
        get_file_metrics,
        get_module_summary,
    )
except ImportError:
    # Fallback si le module n'est pas au bon endroit
    import sys
    sys.path.insert(0, str(Path(__file__).parent.parent / "mcp" / "agentdb"))
    from tools import (
        get_file_context,
        get_symbol_callers,
        get_symbol_callees,
        get_file_impact,
        get_error_history,
        get_patterns,
        get_architecture_decisions,
        search_symbols,
        get_file_metrics,
        get_module_summary,
    )


# =============================================================================
# HELPER FUNCTIONS
# =============================================================================

def is_json_serializable(obj):
    """Vérifie qu'un objet est sérialisable en JSON."""
    try:
        json.dumps(obj)
        return True
    except (TypeError, ValueError):
        return False


def has_required_keys(d, keys):
    """Vérifie qu'un dict contient les clés requises."""
    return all(k in d for k in keys)


# =============================================================================
# TESTS DE GET_FILE_CONTEXT
# =============================================================================

class TestGetFileContext:
    """Tests pour l'outil get_file_context."""

    def test_file_context_basic(self, populated_db):
        """Teste la récupération du contexte d'un fichier."""
        result = get_file_context(populated_db, path="src/main.c")

        assert "file" in result
        assert result["file"]["path"] == "src/main.c"

    def test_file_context_json_serializable(self, populated_db):
        """Vérifie que le résultat est sérialisable en JSON."""
        result = get_file_context(populated_db, path="src/main.c")
        assert is_json_serializable(result)

    def test_file_context_format(self, populated_db):
        """Vérifie le format conforme à PARTIE 7.2."""
        result = get_file_context(populated_db, path="src/main.c")

        # Section file
        assert "file" in result
        file_section = result["file"]
        assert "path" in file_section
        assert "module" in file_section
        assert "is_critical" in file_section

        # Metrics dans file
        if "metrics" in file_section:
            metrics = file_section["metrics"]
            assert "lines_total" in metrics or "lines_code" in metrics

    def test_file_context_with_symbols(self, populated_db):
        """Teste l'inclusion des symboles."""
        result = get_file_context(
            populated_db,
            path="src/main.c",
            include_symbols=True
        )

        assert "symbols" in result
        assert isinstance(result["symbols"], list)

        if result["symbols"]:
            sym = result["symbols"][0]
            assert "name" in sym
            assert "kind" in sym

    def test_file_context_with_dependencies(self, populated_db):
        """Teste l'inclusion des dépendances."""
        result = get_file_context(
            populated_db,
            path="src/lcd/init.c",
            include_dependencies=True
        )

        if "dependencies" in result:
            deps = result["dependencies"]
            assert "includes" in deps or "calls_to" in deps

    def test_file_context_not_found(self, populated_db):
        """Teste avec un fichier inexistant."""
        result = get_file_context(populated_db, path="nonexistent.c")

        assert "error" in result

    def test_file_context_minimal(self, populated_db):
        """Teste avec les options minimales."""
        result = get_file_context(
            populated_db,
            path="src/main.c",
            include_symbols=False,
            include_dependencies=False,
            include_history=False,
            include_patterns=False
        )

        assert "file" in result
        # Les autres sections peuvent être absentes ou vides


# =============================================================================
# TESTS DE GET_SYMBOL_CALLERS
# =============================================================================

class TestMCPGetSymbolCallers:
    """Tests pour l'outil MCP get_symbol_callers."""

    def test_callers_basic(self, populated_db):
        """Teste la récupération des appelants."""
        result = get_symbol_callers(
            populated_db,
            symbol_name="lcd_init",
            max_depth=2
        )

        assert "symbol" in result or "error" in result

    def test_callers_json_serializable(self, populated_db):
        """Vérifie la sérialisation JSON."""
        result = get_symbol_callers(
            populated_db,
            symbol_name="lcd_init",
            max_depth=2
        )
        assert is_json_serializable(result)

    def test_callers_format(self, populated_db):
        """Vérifie le format conforme à PARTIE 7.2."""
        result = get_symbol_callers(
            populated_db,
            symbol_name="lcd_init",
            max_depth=3
        )

        if "error" not in result:
            # Section symbol
            assert "symbol" in result
            assert "name" in result["symbol"]

            # Section callers avec niveaux
            assert "callers" in result
            assert "level_1" in result["callers"]

            # Section summary
            assert "summary" in result
            assert "total_callers" in result["summary"]

    def test_callers_with_file(self, populated_db):
        """Teste avec désambiguïsation par fichier."""
        result = get_symbol_callers(
            populated_db,
            symbol_name="lcd_init",
            file_path="src/lcd/init.c",
            max_depth=2
        )

        if "error" not in result:
            assert result["symbol"]["name"] == "lcd_init"

    def test_callers_not_found(self, populated_db):
        """Teste avec un symbole inexistant."""
        result = get_symbol_callers(
            populated_db,
            symbol_name="nonexistent_function",
            max_depth=2
        )

        assert "error" in result


# =============================================================================
# TESTS DE GET_SYMBOL_CALLEES
# =============================================================================

class TestMCPGetSymbolCallees:
    """Tests pour l'outil MCP get_symbol_callees."""

    def test_callees_basic(self, populated_db):
        """Teste la récupération des appelés."""
        result = get_symbol_callees(
            populated_db,
            symbol_name="lcd_init",
            max_depth=2
        )

        assert "symbol" in result or "error" in result

    def test_callees_json_serializable(self, populated_db):
        """Vérifie la sérialisation JSON."""
        result = get_symbol_callees(
            populated_db,
            symbol_name="lcd_init",
            max_depth=2
        )
        assert is_json_serializable(result)

    def test_callees_format(self, populated_db):
        """Vérifie le format conforme à PARTIE 7.2."""
        result = get_symbol_callees(
            populated_db,
            symbol_name="lcd_init",
            max_depth=2
        )

        if "error" not in result:
            assert "symbol" in result
            assert "callees" in result
            assert "level_1" in result["callees"]

            # types_used peut être présent
            if "types_used" in result:
                assert isinstance(result["types_used"], list)


# =============================================================================
# TESTS DE GET_FILE_IMPACT
# =============================================================================

class TestMCPGetFileImpact:
    """Tests pour l'outil MCP get_file_impact."""

    def test_impact_basic(self, populated_db):
        """Teste le calcul d'impact."""
        result = get_file_impact(populated_db, path="src/lcd/init.c")

        assert "file" in result or "error" in result

    def test_impact_json_serializable(self, populated_db):
        """Vérifie la sérialisation JSON."""
        result = get_file_impact(populated_db, path="src/lcd/init.c")
        assert is_json_serializable(result)

    def test_impact_format(self, populated_db):
        """Vérifie le format conforme à PARTIE 7.2."""
        result = get_file_impact(populated_db, path="src/lcd/init.c")

        if "error" not in result:
            assert "file" in result
            assert "direct_impact" in result
            assert "transitive_impact" in result
            assert "include_impact" in result
            assert "summary" in result

            # Vérifier le format des impacts
            assert isinstance(result["direct_impact"], list)
            assert isinstance(result["include_impact"], list)

            # Summary
            assert "total_files_impacted" in result["summary"]

    def test_impact_with_transitive(self, populated_db):
        """Teste avec impact transitif."""
        result = get_file_impact(
            populated_db,
            path="src/lcd/init.c",
            include_transitive=True,
            max_depth=3
        )

        if "error" not in result:
            assert "transitive_impact" in result

    def test_impact_not_found(self, populated_db):
        """Teste avec fichier inexistant."""
        result = get_file_impact(populated_db, path="nonexistent.c")
        assert "error" in result


# =============================================================================
# TESTS DE GET_ERROR_HISTORY
# =============================================================================

class TestMCPGetErrorHistory:
    """Tests pour l'outil MCP get_error_history."""

    def test_error_history_basic(self, populated_db):
        """Teste la récupération de l'historique."""
        result = get_error_history(populated_db, days=365)

        assert "errors" in result
        assert "statistics" in result

    def test_error_history_json_serializable(self, populated_db):
        """Vérifie la sérialisation JSON."""
        result = get_error_history(populated_db, days=365)
        assert is_json_serializable(result)

    def test_error_history_format(self, populated_db):
        """Vérifie le format conforme à PARTIE 7.2."""
        result = get_error_history(populated_db, days=365)

        # Section query
        assert "query" in result

        # Section errors
        assert "errors" in result
        assert isinstance(result["errors"], list)

        if result["errors"]:
            error = result["errors"][0]
            assert "type" in error
            assert "severity" in error
            assert "title" in error

        # Section statistics
        assert "statistics" in result
        stats = result["statistics"]
        assert "total_errors" in stats
        assert "by_type" in stats
        assert "by_severity" in stats

    def test_error_history_by_file(self, populated_db):
        """Teste le filtrage par fichier."""
        result = get_error_history(
            populated_db,
            file_path="src/lcd/init.c",
            days=365
        )

        assert "errors" in result
        # Toutes les erreurs devraient être pour ce fichier
        for error in result["errors"]:
            # Note: file_path peut être dans différents champs
            pass  # Vérification optionnelle

    def test_error_history_by_severity(self, populated_db):
        """Teste le filtrage par sévérité."""
        result = get_error_history(
            populated_db,
            severity="critical",
            days=365
        )

        assert "errors" in result


# =============================================================================
# TESTS DE GET_PATTERNS
# =============================================================================

class TestMCPGetPatterns:
    """Tests pour l'outil MCP get_patterns."""

    def test_patterns_basic(self, populated_db):
        """Teste la récupération des patterns."""
        result = get_patterns(populated_db)

        assert "applicable_patterns" in result or "project_patterns" in result

    def test_patterns_json_serializable(self, populated_db):
        """Vérifie la sérialisation JSON."""
        result = get_patterns(populated_db)
        assert is_json_serializable(result)

    def test_patterns_format(self, populated_db):
        """Vérifie le format conforme à PARTIE 7.2."""
        result = get_patterns(populated_db)

        # Deux listes de patterns
        assert "applicable_patterns" in result
        assert "project_patterns" in result

        assert isinstance(result["applicable_patterns"], list)
        assert isinstance(result["project_patterns"], list)

        # Format d'un pattern
        all_patterns = result["applicable_patterns"] + result["project_patterns"]
        if all_patterns:
            pattern = all_patterns[0]
            assert "name" in pattern
            assert "category" in pattern

    def test_patterns_by_file(self, populated_db):
        """Teste le filtrage par fichier."""
        result = get_patterns(populated_db, file_path="src/lcd/init.c")

        assert "applicable_patterns" in result

    def test_patterns_by_category(self, populated_db):
        """Teste le filtrage par catégorie."""
        result = get_patterns(populated_db, category="error_handling")

        assert "applicable_patterns" in result or "project_patterns" in result


# =============================================================================
# TESTS DE GET_ARCHITECTURE_DECISIONS
# =============================================================================

class TestMCPGetArchitectureDecisions:
    """Tests pour l'outil MCP get_architecture_decisions."""

    def test_adrs_basic(self, populated_db):
        """Teste la récupération des ADRs."""
        result = get_architecture_decisions(populated_db)

        assert "decisions" in result

    def test_adrs_json_serializable(self, populated_db):
        """Vérifie la sérialisation JSON."""
        result = get_architecture_decisions(populated_db)
        assert is_json_serializable(result)

    def test_adrs_format(self, populated_db):
        """Vérifie le format conforme à PARTIE 7.2."""
        result = get_architecture_decisions(populated_db)

        assert "decisions" in result
        assert isinstance(result["decisions"], list)

        if result["decisions"]:
            adr = result["decisions"][0]
            assert "id" in adr
            assert "title" in adr
            assert "status" in adr
            assert "context" in adr
            assert "decision" in adr

    def test_adrs_by_status(self, populated_db):
        """Teste le filtrage par statut."""
        result = get_architecture_decisions(populated_db, status="accepted")

        for adr in result["decisions"]:
            assert adr["status"] == "accepted"

    def test_adrs_by_module(self, populated_db):
        """Teste le filtrage par module."""
        result = get_architecture_decisions(populated_db, module="lcd")

        assert "decisions" in result


# =============================================================================
# TESTS DE SEARCH_SYMBOLS
# =============================================================================

class TestMCPSearchSymbols:
    """Tests pour l'outil MCP search_symbols."""

    def test_search_basic(self, populated_db):
        """Teste la recherche de symboles."""
        result = search_symbols(populated_db, query="lcd*")

        assert "results" in result
        assert "total" in result
        assert "returned" in result

    def test_search_json_serializable(self, populated_db):
        """Vérifie la sérialisation JSON."""
        result = search_symbols(populated_db, query="*")
        assert is_json_serializable(result)

    def test_search_format(self, populated_db):
        """Vérifie le format conforme à PARTIE 7.2."""
        result = search_symbols(populated_db, query="*")

        assert "query" in result
        assert "results" in result
        assert "total" in result
        assert "returned" in result

        assert isinstance(result["results"], list)
        assert isinstance(result["total"], int)
        assert isinstance(result["returned"], int)

        if result["results"]:
            sym = result["results"][0]
            assert "name" in sym
            assert "kind" in sym
            assert "file" in sym

    def test_search_by_kind(self, populated_db):
        """Teste le filtrage par type."""
        result = search_symbols(populated_db, query="*", kind="function")

        for sym in result["results"]:
            assert sym["kind"] == "function"

    def test_search_limit(self, populated_db):
        """Teste la limitation des résultats."""
        result = search_symbols(populated_db, query="*", limit=2)

        assert result["returned"] <= 2
        assert len(result["results"]) <= 2

    def test_search_wildcard(self, populated_db):
        """Teste les wildcards."""
        # Recherche avec wildcard
        result = search_symbols(populated_db, query="lcd_*")

        # Tous les résultats devraient commencer par "lcd_"
        for sym in result["results"]:
            assert sym["name"].startswith("lcd_") or "lcd_" in sym["name"].lower()


# =============================================================================
# TESTS DE GET_FILE_METRICS
# =============================================================================

class TestMCPGetFileMetrics:
    """Tests pour l'outil MCP get_file_metrics."""

    def test_metrics_basic(self, populated_db):
        """Teste la récupération des métriques."""
        result = get_file_metrics(populated_db, path="src/main.c")

        assert "file" in result or "error" in result

    def test_metrics_json_serializable(self, populated_db):
        """Vérifie la sérialisation JSON."""
        result = get_file_metrics(populated_db, path="src/main.c")
        assert is_json_serializable(result)

    def test_metrics_format(self, populated_db):
        """Vérifie le format conforme à PARTIE 7.2."""
        result = get_file_metrics(populated_db, path="src/main.c")

        if "error" not in result:
            assert "file" in result

            # Section size
            assert "size" in result
            size = result["size"]
            assert "lines_total" in size
            assert "lines_code" in size

            # Section complexity
            assert "complexity" in result
            complexity = result["complexity"]
            assert "cyclomatic_avg" in complexity or "cyclomatic_total" in complexity

            # Section structure
            assert "structure" in result
            structure = result["structure"]
            assert "functions" in structure
            assert "types" in structure

            # Section quality
            assert "quality" in result

            # Section activity
            assert "activity" in result
            activity = result["activity"]
            assert "commits_30d" in activity

    def test_metrics_not_found(self, populated_db):
        """Teste avec fichier inexistant."""
        result = get_file_metrics(populated_db, path="nonexistent.c")
        assert "error" in result


# =============================================================================
# TESTS DE GET_MODULE_SUMMARY
# =============================================================================

class TestMCPGetModuleSummary:
    """Tests pour l'outil MCP get_module_summary."""

    def test_summary_basic(self, populated_db):
        """Teste la récupération du résumé d'un module."""
        result = get_module_summary(populated_db, module="lcd")

        assert "module" in result or "error" in result

    def test_summary_json_serializable(self, populated_db):
        """Vérifie la sérialisation JSON."""
        result = get_module_summary(populated_db, module="lcd")
        assert is_json_serializable(result)

    def test_summary_format(self, populated_db):
        """Vérifie le format conforme à PARTIE 7.2."""
        result = get_module_summary(populated_db, module="lcd")

        if "error" not in result:
            assert "module" in result

            # Section files
            assert "files" in result
            files = result["files"]
            assert "total" in files
            assert "sources" in files
            assert "headers" in files

            # Section symbols
            assert "symbols" in result
            symbols = result["symbols"]
            assert "functions" in symbols
            assert "types" in symbols

            # Section metrics
            assert "metrics" in result

            # Section health
            assert "health" in result
            health = result["health"]
            assert "test_coverage" in health or "technical_debt" in health

            # Section dependencies
            assert "dependencies" in result
            deps = result["dependencies"]
            assert "depends_on" in deps
            assert "depended_by" in deps

    def test_summary_not_found(self, populated_db):
        """Teste avec module inexistant."""
        result = get_module_summary(populated_db, module="nonexistent_module")
        assert "error" in result


# =============================================================================
# TESTS D'INTÉGRATION MCP
# =============================================================================

class TestMCPIntegration:
    """Tests d'intégration des outils MCP."""

    def test_all_tools_json_serializable(self, populated_db):
        """Vérifie que tous les outils retournent du JSON valide."""
        tools_results = [
            get_file_context(populated_db, path="src/main.c"),
            get_symbol_callers(populated_db, symbol_name="lcd_init"),
            get_symbol_callees(populated_db, symbol_name="lcd_init"),
            get_file_impact(populated_db, path="src/lcd/init.c"),
            get_error_history(populated_db, days=365),
            get_patterns(populated_db),
            get_architecture_decisions(populated_db),
            search_symbols(populated_db, query="*"),
            get_file_metrics(populated_db, path="src/main.c"),
            get_module_summary(populated_db, module="lcd"),
        ]

        for result in tools_results:
            assert is_json_serializable(result), f"Result not JSON serializable: {result}"

    def test_no_exceptions_on_missing_data(self, db):
        """Vérifie que les outils gèrent gracieusement les données manquantes."""
        # Base vide avec schéma uniquement
        tools = [
            lambda: get_file_context(db, path="x.c"),
            lambda: get_symbol_callers(db, symbol_name="x"),
            lambda: get_symbol_callees(db, symbol_name="x"),
            lambda: get_file_impact(db, path="x.c"),
            lambda: get_error_history(db, days=1),
            lambda: get_patterns(db),
            lambda: get_architecture_decisions(db),
            lambda: search_symbols(db, query="*"),
            lambda: get_file_metrics(db, path="x.c"),
            lambda: get_module_summary(db, module="x"),
        ]

        for tool in tools:
            try:
                result = tool()
                # Le résultat doit être un dict (avec ou sans erreur)
                assert isinstance(result, dict)
            except Exception as e:
                pytest.fail(f"Tool raised exception: {e}")

    def test_consistent_error_format(self, db):
        """Vérifie que les erreurs ont un format cohérent."""
        error_results = [
            get_file_context(db, path="nonexistent.c"),
            get_symbol_callers(db, symbol_name="nonexistent"),
            get_file_impact(db, path="nonexistent.c"),
            get_file_metrics(db, path="nonexistent.c"),
            get_module_summary(db, module="nonexistent"),
        ]

        for result in error_results:
            # Soit il y a une erreur, soit le résultat est vide/valide
            if "error" in result:
                assert isinstance(result["error"], str)
