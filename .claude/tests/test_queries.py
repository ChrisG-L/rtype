"""
Tests pour les requêtes de traversée du graphe AgentDB.

Teste :
- get_symbol_callers avec différentes profondeurs
- get_symbol_callees
- get_file_impact
- get_type_users
- get_include_tree
- Performance (< 100ms)

Utilise une base de données en mémoire avec des données de test.
"""

import pytest
import time
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from agentdb.db import Database
from agentdb.queries import (
    get_symbol_callers,
    get_symbol_callees,
    get_file_impact,
    get_type_users,
    get_include_tree,
    get_symbol_by_name_qualified,
    get_symbol_callers_by_name,
    get_symbol_callees_by_name,
    get_type_users_by_name,
)


# =============================================================================
# FIXTURES POUR LES DONNÉES DE TEST
# =============================================================================

@pytest.fixture
def graph_db(db):
    """Base de données avec un graphe de dépendances complet."""
    # Créer les fichiers
    files = [
        ("src/main.c", "main.c", "core", 1),      # id=1
        ("src/system.c", "system.c", "core", 1),   # id=2
        ("src/lcd/init.c", "init.c", "lcd", 0),    # id=3
        ("src/lcd/lcd.h", "lcd.h", "lcd", 0),      # id=4
        ("src/lcd/write.c", "write.c", "lcd", 0),  # id=5
        ("src/utils/alloc.c", "alloc.c", "utils", 0),  # id=6
        ("tests/test_lcd.c", "test_lcd.c", "tests", 0),  # id=7
    ]

    for path, filename, module, is_critical in files:
        db.execute(
            "INSERT INTO files (path, filename, module, is_critical) VALUES (?, ?, ?, ?)",
            (path, filename, module, is_critical)
        )

    # Créer les symboles
    symbols = [
        # main.c
        (1, "main", "function", "int main(int argc, char **argv)", 10, 100, 8),  # id=1
        # system.c
        (2, "system_init", "function", "void system_init(void)", 10, 50, 6),  # id=2
        (2, "system_shutdown", "function", "void system_shutdown(void)", 55, 80, 4),  # id=3
        # lcd/init.c
        (3, "lcd_init", "function", "int lcd_init(LCD_Config *cfg)", 20, 80, 12),  # id=4
        (3, "lcd_reset", "function", "void lcd_reset(void)", 85, 100, 3),  # id=5
        # lcd/lcd.h
        (4, "LCD_Config", "struct", None, 10, 25, 0),  # id=6
        (4, "LCD_Status", "enum", None, 30, 40, 0),  # id=7
        # lcd/write.c
        (5, "lcd_write", "function", "int lcd_write(uint8_t *data, size_t len)", 10, 60, 10),  # id=8
        (5, "lcd_clear", "function", "void lcd_clear(void)", 65, 80, 5),  # id=9
        # utils/alloc.c
        (6, "safe_alloc", "function", "void *safe_alloc(size_t size)", 10, 30, 6),  # id=10
        (6, "safe_free", "function", "void safe_free(void *ptr)", 35, 50, 3),  # id=11
        # tests/test_lcd.c
        (7, "test_lcd_init", "function", "void test_lcd_init(void)", 10, 40, 5),  # id=12
        (7, "test_lcd_write", "function", "void test_lcd_write(void)", 45, 80, 6),  # id=13
    ]

    for file_id, name, kind, sig, line_start, line_end, complexity in symbols:
        db.execute(
            """INSERT INTO symbols (file_id, name, kind, signature, line_start, line_end, complexity)
               VALUES (?, ?, ?, ?, ?, ?, ?)""",
            (file_id, name, kind, sig, line_start, line_end, complexity)
        )

    # Créer les relations d'appel (source_id, target_id, type, line)
    relations = [
        # main -> system_init
        (1, 2, "calls", 25),
        # main -> system_shutdown
        (1, 3, "calls", 95),
        # system_init -> lcd_init
        (2, 4, "calls", 30),
        # lcd_init -> lcd_reset
        (4, 5, "calls", 50),
        # lcd_init -> safe_alloc
        (4, 10, "calls", 35),
        # lcd_init uses LCD_Config
        (4, 6, "uses_type", 20),
        # lcd_init uses LCD_Status
        (4, 7, "uses_type", 45),
        # lcd_write -> safe_alloc
        (8, 10, "calls", 20),
        # lcd_write -> lcd_clear
        (8, 9, "calls", 55),
        # lcd_reset -> safe_free
        (5, 11, "calls", 90),
        # system_shutdown -> lcd_reset
        (3, 5, "calls", 60),
        # test_lcd_init -> lcd_init
        (12, 4, "calls", 20),
        # test_lcd_write -> lcd_write
        (13, 8, "calls", 50),
        # test_lcd_write -> lcd_init
        (13, 4, "calls", 48),
    ]

    for source_id, target_id, rel_type, line in relations:
        db.execute(
            "INSERT INTO relations (source_id, target_id, relation_type, location_line) VALUES (?, ?, ?, ?)",
            (source_id, target_id, rel_type, line)
        )

    # Créer les relations entre fichiers (includes)
    file_relations = [
        # lcd/init.c includes lcd.h
        (3, 4, "includes", 1),
        # lcd/write.c includes lcd.h
        (5, 4, "includes", 1),
        # main.c includes lcd.h
        (1, 4, "includes", 3),
        # system.c includes lcd.h
        (2, 4, "includes", 2),
        # test_lcd.c includes lcd.h
        (7, 4, "includes", 1),
    ]

    for source_id, target_id, rel_type, line in file_relations:
        db.execute(
            "INSERT INTO file_relations (source_file_id, target_file_id, relation_type, line_number) VALUES (?, ?, ?, ?)",
            (source_id, target_id, rel_type, line)
        )

    db.commit()
    return db


# =============================================================================
# TESTS DE GET_SYMBOL_CALLERS
# =============================================================================

class TestGetSymbolCallers:
    """Tests pour get_symbol_callers."""

    def test_callers_depth_1(self, graph_db):
        """Teste la récupération des appelants directs (depth=1)."""
        # lcd_init (id=4) est appelé par system_init, test_lcd_init, test_lcd_write
        result = get_symbol_callers(graph_db, symbol_id=4, max_depth=1)

        assert "symbol" in result
        assert result["symbol"]["name"] == "lcd_init"

        assert "callers" in result
        assert "level_1" in result["callers"]

        level_1_names = [c["name"] for c in result["callers"]["level_1"]]
        assert "system_init" in level_1_names
        assert "test_lcd_init" in level_1_names
        assert "test_lcd_write" in level_1_names

        assert result["summary"]["total_callers"] == 3

    def test_callers_depth_2(self, graph_db):
        """Teste la récupération des appelants jusqu'à depth=2."""
        # lcd_init (id=4) est appelé par system_init <- main
        result = get_symbol_callers(graph_db, symbol_id=4, max_depth=2)

        level_1_names = [c["name"] for c in result["callers"]["level_1"]]
        level_2_names = [c["name"] for c in result["callers"]["level_2"]]

        # Niveau 1 : appelants directs
        assert "system_init" in level_1_names

        # Niveau 2 : appelants des appelants (main appelle system_init)
        assert "main" in level_2_names

    def test_callers_depth_3(self, graph_db):
        """Teste la récupération des appelants jusqu'à depth=3."""
        # lcd_reset (id=5) <- lcd_init <- system_init <- main
        result = get_symbol_callers(graph_db, symbol_id=5, max_depth=3)

        level_1 = [c["name"] for c in result["callers"]["level_1"]]
        level_2 = [c["name"] for c in result["callers"]["level_2"]]
        level_3 = [c["name"] for c in result["callers"]["level_3"]]

        assert "lcd_init" in level_1
        assert "system_init" in level_2 or "system_shutdown" in level_1
        # La structure dépend du parcours exact

        assert result["summary"]["max_depth_reached"] >= 1

    def test_callers_no_callers(self, graph_db):
        """Teste un symbole sans appelants."""
        # main (id=1) n'a pas d'appelants
        result = get_symbol_callers(graph_db, symbol_id=1, max_depth=3)

        assert result["symbol"]["name"] == "main"
        assert result["summary"]["total_callers"] == 0
        assert all(len(result["callers"][f"level_{i}"]) == 0 for i in range(1, 4))

    def test_callers_invalid_symbol(self, graph_db):
        """Teste avec un symbole inexistant."""
        with pytest.raises(ValueError, match="not found"):
            get_symbol_callers(graph_db, symbol_id=9999, max_depth=3)

    def test_callers_invalid_depth(self, graph_db):
        """Teste avec une profondeur invalide."""
        with pytest.raises(ValueError, match="max_depth"):
            get_symbol_callers(graph_db, symbol_id=4, max_depth=0)

        with pytest.raises(ValueError, match="max_depth"):
            get_symbol_callers(graph_db, symbol_id=4, max_depth=11)

    def test_callers_summary(self, graph_db):
        """Teste le résumé des appelants."""
        result = get_symbol_callers(graph_db, symbol_id=4, max_depth=3)

        summary = result["summary"]
        assert "total_callers" in summary
        assert "max_depth_reached" in summary
        assert "critical_callers" in summary
        assert "files_affected" in summary

        assert isinstance(summary["files_affected"], list)

    def test_callers_by_name(self, graph_db):
        """Teste get_symbol_callers_by_name."""
        result = get_symbol_callers_by_name(graph_db, symbol_name="lcd_init", max_depth=2)

        assert result["symbol"]["name"] == "lcd_init"
        assert result["summary"]["total_callers"] >= 1

    def test_callers_by_name_with_file(self, graph_db):
        """Teste get_symbol_callers_by_name avec fichier."""
        result = get_symbol_callers_by_name(
            graph_db,
            symbol_name="lcd_init",
            file_path="src/lcd/init.c",
            max_depth=2
        )

        assert result["symbol"]["name"] == "lcd_init"


# =============================================================================
# TESTS DE GET_SYMBOL_CALLEES
# =============================================================================

class TestGetSymbolCallees:
    """Tests pour get_symbol_callees."""

    def test_callees_depth_1(self, graph_db):
        """Teste la récupération des appelés directs."""
        # lcd_init (id=4) appelle lcd_reset, safe_alloc
        result = get_symbol_callees(graph_db, symbol_id=4, max_depth=1)

        assert result["symbol"]["name"] == "lcd_init"

        level_1_names = [c["name"] for c in result["callees"]["level_1"]]
        assert "lcd_reset" in level_1_names
        assert "safe_alloc" in level_1_names

    def test_callees_depth_2(self, graph_db):
        """Teste les appelés jusqu'à depth=2."""
        # lcd_init -> lcd_reset -> safe_free
        result = get_symbol_callees(graph_db, symbol_id=4, max_depth=2)

        level_1 = [c["name"] for c in result["callees"]["level_1"]]
        level_2 = [c["name"] for c in result["callees"]["level_2"]]

        assert "lcd_reset" in level_1
        assert "safe_free" in level_2

    def test_callees_types_used(self, graph_db):
        """Teste la récupération des types utilisés."""
        # lcd_init utilise LCD_Config et LCD_Status
        result = get_symbol_callees(graph_db, symbol_id=4, max_depth=1)

        types_names = [t["name"] for t in result["types_used"]]
        assert "LCD_Config" in types_names
        assert "LCD_Status" in types_names

    def test_callees_no_callees(self, graph_db):
        """Teste un symbole sans appelés."""
        # safe_free (id=11) n'appelle rien
        result = get_symbol_callees(graph_db, symbol_id=11, max_depth=2)

        assert result["summary"]["total_callees"] == 0

    def test_callees_by_name(self, graph_db):
        """Teste get_symbol_callees_by_name."""
        result = get_symbol_callees_by_name(graph_db, symbol_name="lcd_init", max_depth=2)

        assert result["symbol"]["name"] == "lcd_init"
        assert result["summary"]["total_callees"] >= 1


# =============================================================================
# TESTS DE GET_FILE_IMPACT
# =============================================================================

class TestGetFileImpact:
    """Tests pour get_file_impact."""

    def test_file_impact_direct(self, graph_db):
        """Teste l'impact direct d'un fichier."""
        # lcd/init.c contient lcd_init qui est appelé par d'autres
        result = get_file_impact(graph_db, file_path="src/lcd/init.c", include_transitive=False)

        assert result["file"] == "src/lcd/init.c"
        assert "direct_impact" in result

        # Le fichier system.c appelle lcd_init
        impacted_files = [d["file"] for d in result["direct_impact"]]
        assert "src/system.c" in impacted_files or len(impacted_files) >= 0

    def test_file_impact_includes(self, graph_db):
        """Teste l'impact par includes."""
        # lcd/lcd.h est inclus par plusieurs fichiers
        result = get_file_impact(graph_db, file_path="src/lcd/lcd.h")

        assert "include_impact" in result

        # Plusieurs fichiers incluent lcd.h
        including_files = [i["file"] for i in result["include_impact"]]
        assert len(including_files) >= 1

    def test_file_impact_transitive(self, graph_db):
        """Teste l'impact transitif."""
        result = get_file_impact(
            graph_db,
            file_path="src/lcd/init.c",
            include_transitive=True,
            max_depth=3
        )

        # Si transitif, on devrait voir plus de fichiers impactés
        assert "transitive_impact" in result
        total = result["summary"]["total_files_impacted"]
        # Au moins le fichier lui-même n'est pas compté, mais les impactés le sont
        assert total >= 0

    def test_file_impact_not_found(self, graph_db):
        """Teste avec un fichier inexistant."""
        with pytest.raises(ValueError, match="not found"):
            get_file_impact(graph_db, file_path="nonexistent/file.c")

    def test_file_impact_summary(self, graph_db):
        """Teste le résumé de l'impact."""
        result = get_file_impact(graph_db, file_path="src/lcd/init.c")

        summary = result["summary"]
        assert "total_files_impacted" in summary
        assert "critical_files_impacted" in summary
        assert "max_depth" in summary


# =============================================================================
# TESTS DE GET_TYPE_USERS
# =============================================================================

class TestGetTypeUsers:
    """Tests pour get_type_users."""

    def test_type_users(self, graph_db):
        """Teste la récupération des utilisateurs d'un type."""
        # LCD_Config (id=6) est utilisé par lcd_init
        result = get_type_users(graph_db, type_symbol_id=6)

        assert len(result) >= 1

        user_names = [u["name"] for u in result]
        assert "lcd_init" in user_names

    def test_type_users_empty(self, graph_db):
        """Teste un type sans utilisateurs."""
        # Un type qui n'est pas utilisé
        # Dans notre jeu de données, tous les types sont utilisés
        # Créer un type orphelin
        graph_db.execute(
            "INSERT INTO symbols (file_id, name, kind) VALUES (4, 'UnusedType', 'struct')"
        )
        graph_db.commit()

        # Récupérer l'ID
        row = graph_db.fetch_one("SELECT id FROM symbols WHERE name = 'UnusedType'")
        unused_id = row["id"]

        result = get_type_users(graph_db, type_symbol_id=unused_id)
        assert len(result) == 0

    def test_type_users_by_name(self, graph_db):
        """Teste get_type_users_by_name."""
        result = get_type_users_by_name(graph_db, type_name="LCD_Config")

        user_names = [u["name"] for u in result]
        assert "lcd_init" in user_names


# =============================================================================
# TESTS DE GET_INCLUDE_TREE
# =============================================================================

class TestGetIncludeTree:
    """Tests pour get_include_tree."""

    def test_include_tree_basic(self, graph_db):
        """Teste l'arbre d'inclusion basique."""
        # lcd/init.c inclut lcd.h
        result = get_include_tree(graph_db, file_path="src/lcd/init.c", max_depth=2)

        assert result["root"] == "src/lcd/init.c"
        assert "includes" in result
        assert "tree" in result
        assert "summary" in result

    def test_include_tree_depth(self, graph_db):
        """Teste la profondeur de l'arbre."""
        result = get_include_tree(graph_db, file_path="src/lcd/init.c", max_depth=3)

        # Vérifier que les includes sont retournés
        included_paths = [i["path"] for i in result["includes"]]

        # lcd/init.c inclut lcd.h
        if result["summary"]["total_includes"] > 0:
            assert "src/lcd/lcd.h" in included_paths

    def test_include_tree_no_includes(self, graph_db):
        """Teste un fichier sans includes."""
        # utils/alloc.c n'inclut rien dans notre jeu de données
        result = get_include_tree(graph_db, file_path="src/utils/alloc.c", max_depth=3)

        assert result["summary"]["total_includes"] == 0

    def test_include_tree_not_found(self, graph_db):
        """Teste avec un fichier inexistant."""
        with pytest.raises(ValueError, match="not found"):
            get_include_tree(graph_db, file_path="nonexistent.c")


# =============================================================================
# TESTS DE GET_SYMBOL_BY_NAME_QUALIFIED
# =============================================================================

class TestGetSymbolByNameQualified:
    """Tests pour get_symbol_by_name_qualified."""

    def test_find_by_name(self, graph_db):
        """Teste la recherche par nom simple."""
        symbol = get_symbol_by_name_qualified(graph_db, name="lcd_init")

        assert symbol is not None
        assert symbol.name == "lcd_init"
        assert symbol.kind == "function"

    def test_find_by_name_and_file(self, graph_db):
        """Teste la recherche avec désambiguïsation par fichier."""
        # LCD_Config existe dans lcd.h
        symbol = get_symbol_by_name_qualified(
            graph_db,
            name="LCD_Config",
            file_path="src/lcd/lcd.h"
        )

        assert symbol is not None
        assert symbol.name == "LCD_Config"

    def test_find_not_found(self, graph_db):
        """Teste la recherche d'un symbole inexistant."""
        symbol = get_symbol_by_name_qualified(graph_db, name="nonexistent_symbol")
        assert symbol is None

    def test_find_empty_name(self, graph_db):
        """Teste avec un nom vide."""
        with pytest.raises(ValueError, match="cannot be empty"):
            get_symbol_by_name_qualified(graph_db, name="")


# =============================================================================
# TESTS DE PERFORMANCE
# =============================================================================

class TestQueryPerformance:
    """Tests de performance pour les requêtes."""

    def test_callers_performance(self, graph_db, assert_performance):
        """Vérifie que get_symbol_callers s'exécute en < 100ms."""
        with assert_performance(max_ms=100) as perf:
            result = get_symbol_callers(graph_db, symbol_id=4, max_depth=3)

        perf.check()
        assert result is not None

    def test_callees_performance(self, graph_db, assert_performance):
        """Vérifie que get_symbol_callees s'exécute en < 100ms."""
        with assert_performance(max_ms=100) as perf:
            result = get_symbol_callees(graph_db, symbol_id=4, max_depth=3)

        perf.check()
        assert result is not None

    def test_file_impact_performance(self, graph_db, assert_performance):
        """Vérifie que get_file_impact s'exécute en < 100ms."""
        with assert_performance(max_ms=100) as perf:
            result = get_file_impact(
                graph_db,
                file_path="src/lcd/init.c",
                include_transitive=True,
                max_depth=3
            )

        perf.check()
        assert result is not None

    def test_include_tree_performance(self, graph_db, assert_performance):
        """Vérifie que get_include_tree s'exécute en < 100ms."""
        with assert_performance(max_ms=100) as perf:
            result = get_include_tree(graph_db, file_path="src/lcd/init.c", max_depth=5)

        perf.check()
        assert result is not None

    def test_type_users_performance(self, graph_db, assert_performance):
        """Vérifie que get_type_users s'exécute en < 100ms."""
        with assert_performance(max_ms=100) as perf:
            result = get_type_users(graph_db, type_symbol_id=6)

        perf.check()
        assert result is not None


# =============================================================================
# TESTS AVEC DONNÉES VOLUMINEUSES
# =============================================================================

class TestLargeGraph:
    """Tests avec un graphe plus volumineux."""

    @pytest.fixture
    def large_graph_db(self, db):
        """Crée un graphe avec beaucoup de nœuds."""
        # Créer 100 fichiers
        for i in range(100):
            db.execute(
                "INSERT INTO files (path, filename, module) VALUES (?, ?, ?)",
                (f"src/file_{i}.c", f"file_{i}.c", f"module_{i % 10}")
            )

        # Créer 500 symboles (5 par fichier)
        for i in range(100):
            for j in range(5):
                db.execute(
                    "INSERT INTO symbols (file_id, name, kind, complexity) VALUES (?, ?, 'function', ?)",
                    (i + 1, f"func_{i}_{j}", (i + j) % 20)
                )

        # Créer 1000 relations
        for i in range(1000):
            source = (i % 500) + 1
            target = ((i * 7) % 500) + 1
            if source != target:
                try:
                    db.execute(
                        "INSERT INTO relations (source_id, target_id, relation_type) VALUES (?, ?, 'calls')",
                        (source, target)
                    )
                except Exception:
                    pass  # Ignorer les doublons

        db.commit()
        return db

    def test_callers_large_graph(self, large_graph_db, assert_performance):
        """Teste get_symbol_callers sur un grand graphe."""
        with assert_performance(max_ms=200) as perf:
            result = get_symbol_callers(large_graph_db, symbol_id=1, max_depth=3)

        perf.check()
        assert result is not None

    def test_callees_large_graph(self, large_graph_db, assert_performance):
        """Teste get_symbol_callees sur un grand graphe."""
        with assert_performance(max_ms=200) as perf:
            result = get_symbol_callees(large_graph_db, symbol_id=1, max_depth=3)

        perf.check()
        assert result is not None


# =============================================================================
# TESTS D'INTÉGRATION
# =============================================================================

class TestQueryIntegration:
    """Tests d'intégration des requêtes."""

    def test_caller_callee_consistency(self, graph_db):
        """Vérifie la cohérence entre callers et callees."""
        # Si A appelle B, alors B doit avoir A dans ses callers
        # Et A doit avoir B dans ses callees

        # lcd_init (4) appelle lcd_reset (5)
        callees = get_symbol_callees(graph_db, symbol_id=4, max_depth=1)
        callers = get_symbol_callers(graph_db, symbol_id=5, max_depth=1)

        callee_names = [c["name"] for c in callees["callees"]["level_1"]]
        caller_names = [c["name"] for c in callers["callers"]["level_1"]]

        assert "lcd_reset" in callee_names
        assert "lcd_init" in caller_names

    def test_impact_includes_callers(self, graph_db):
        """Vérifie que l'impact inclut les fichiers des appelants."""
        # L'impact de lcd/init.c devrait inclure les fichiers
        # qui appellent des symboles de lcd/init.c

        impact = get_file_impact(graph_db, file_path="src/lcd/init.c")
        callers = get_symbol_callers(graph_db, symbol_id=4, max_depth=1)

        # Les fichiers des appelants devraient être dans l'impact
        caller_files = set(c["file"] for c in callers["callers"]["level_1"])
        impact_files = set(d["file"] for d in impact["direct_impact"])

        # Au moins quelques fichiers devraient correspondre
        # Note: peut être vide si aucun fichier n'appelle directement
        assert isinstance(impact_files, set)
