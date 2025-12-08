"""
AgentDB - Requêtes complexes de traversée du graphe.

Ce module contient les requêtes avancées pour traverser le graphe de dépendances :
- get_symbol_callers : Trouve tous les appelants d'un symbole (récursif)
- get_symbol_callees : Trouve tous les symboles appelés par un symbole (récursif)
- get_file_impact : Calcule l'impact de la modification d'un fichier
- get_type_users : Trouve les utilisateurs d'un type
- get_include_tree : Récupère l'arbre d'inclusion d'un fichier
- get_symbol_by_name_qualified : Recherche un symbole par nom avec désambiguïsation

Ces requêtes utilisent des CTE récursives pour parcourir le graphe de dépendances.

Usage:
    from agentdb.db import get_database
    from agentdb.queries import (
        get_symbol_callers,
        get_symbol_callees,
        get_file_impact,
        get_type_users,
        get_include_tree,
        get_symbol_by_name_qualified,
    )

    db = get_database()

    # Trouver tous les appelants jusqu'à 3 niveaux
    result = get_symbol_callers(db, symbol_id=42, max_depth=3)
    print(f"Total callers: {result['summary']['total_callers']}")

    # Calculer l'impact d'un fichier
    impact = get_file_impact(db, "src/lcd/init.c")
    print(f"Files affected: {impact['summary']['total_files_impacted']}")
"""

from __future__ import annotations

import logging
import time
from functools import wraps
from typing import Any, Optional

from .db import Database
from .models import Symbol, File, CallerInfo, ImpactAnalysis

# Configuration du logging
logger = logging.getLogger("agentdb.queries")

# Seuil pour logger les requêtes lentes (ms)
SLOW_QUERY_THRESHOLD_MS = 100


# =============================================================================
# HELPERS
# =============================================================================

def _log_slow_query(query_name: str, duration_ms: float, params: dict[str, Any]) -> None:
    """Log une requête lente si elle dépasse le seuil."""
    if duration_ms > SLOW_QUERY_THRESHOLD_MS:
        logger.warning(
            f"Slow query: {query_name} took {duration_ms:.1f}ms "
            f"(threshold: {SLOW_QUERY_THRESHOLD_MS}ms) - params: {params}"
        )


def _timed_query(func):
    """Décorateur pour mesurer le temps d'exécution des requêtes."""
    @wraps(func)
    def wrapper(*args, **kwargs):
        start = time.perf_counter()
        result = func(*args, **kwargs)
        duration_ms = (time.perf_counter() - start) * 1000
        # Extraire les paramètres pertinents pour le log
        params = {k: v for k, v in kwargs.items() if k != 'db'}
        _log_slow_query(func.__name__, duration_ms, params)
        return result
    return wrapper


# =============================================================================
# SQL TEMPLATES
# =============================================================================

# Requête récursive pour trouver les appelants
SQL_GET_CALLERS = """
WITH RECURSIVE callers AS (
    -- Cas de base : appelants directs
    SELECT
        s.id,
        s.name,
        s.kind,
        f.path as file_path,
        f.is_critical,
        r.location_line,
        r.is_direct,
        1 as depth
    FROM symbols s
    JOIN relations r ON r.source_id = s.id
    JOIN files f ON s.file_id = f.id
    WHERE r.target_id = :symbol_id
    AND r.relation_type = 'calls'

    UNION ALL

    -- Cas récursif : appelants des appelants
    SELECT
        s.id,
        s.name,
        s.kind,
        f.path as file_path,
        f.is_critical,
        r.location_line,
        r.is_direct,
        c.depth + 1 as depth
    FROM symbols s
    JOIN relations r ON r.source_id = s.id
    JOIN files f ON s.file_id = f.id
    JOIN callers c ON r.target_id = c.id
    WHERE r.relation_type = 'calls'
    AND c.depth < :max_depth
)
SELECT DISTINCT id, name, kind, file_path, is_critical, location_line, is_direct, depth
FROM callers
ORDER BY depth, name;
"""

# Requête récursive pour trouver les appelés
SQL_GET_CALLEES = """
WITH RECURSIVE callees AS (
    SELECT
        s.id,
        s.name,
        s.kind,
        f.path as file_path,
        f.is_critical,
        r.location_line,
        1 as depth
    FROM symbols s
    JOIN relations r ON r.target_id = s.id
    JOIN files f ON s.file_id = f.id
    WHERE r.source_id = :symbol_id
    AND r.relation_type = 'calls'

    UNION ALL

    SELECT
        s.id,
        s.name,
        s.kind,
        f.path as file_path,
        f.is_critical,
        r.location_line,
        c.depth + 1 as depth
    FROM symbols s
    JOIN relations r ON r.target_id = s.id
    JOIN files f ON s.file_id = f.id
    JOIN callees c ON r.source_id = c.id
    WHERE r.relation_type = 'calls'
    AND c.depth < :max_depth
)
SELECT DISTINCT id, name, kind, file_path, is_critical, location_line, depth
FROM callees
ORDER BY depth, name;
"""

# Requête pour les types utilisés par un symbole
SQL_GET_TYPES_USED = """
SELECT DISTINCT
    s.id,
    s.name,
    s.kind,
    f.path as file_path,
    r.relation_type
FROM symbols s
JOIN relations r ON r.target_id = s.id
JOIN files f ON s.file_id = f.id
WHERE r.source_id = :symbol_id
AND r.relation_type IN ('uses_type', 'returns_type', 'has_param_type', 'instantiates')
ORDER BY s.name;
"""

# Requête pour l'impact d'un fichier par includes
SQL_FILE_IMPACT_BY_INCLUDES = """
SELECT DISTINCT
    f2.id,
    f2.path,
    f2.is_critical,
    'includes' as reason,
    fr.line_number
FROM files f1
JOIN file_relations fr ON fr.target_file_id = f1.id
JOIN files f2 ON fr.source_file_id = f2.id
WHERE f1.path = :file_path
AND fr.relation_type IN ('includes', 'imports');
"""

# Requête pour l'impact d'un fichier par calls
SQL_FILE_IMPACT_BY_CALLS = """
SELECT DISTINCT
    f2.id,
    f2.path,
    f2.is_critical,
    s2.name as symbol_name,
    s1.name as called_symbol,
    r.location_line,
    'calls' as reason
FROM files f1
JOIN symbols s1 ON s1.file_id = f1.id
JOIN relations r ON r.target_id = s1.id
JOIN symbols s2 ON r.source_id = s2.id
JOIN files f2 ON s2.file_id = f2.id
WHERE f1.path = :file_path
AND r.relation_type = 'calls'
AND f2.id != f1.id;
"""

# Requête récursive pour l'arbre d'includes
SQL_INCLUDE_TREE = """
WITH RECURSIVE include_tree AS (
    SELECT
        f.id,
        f.path,
        0 as depth,
        f.path as root_path
    FROM files f
    WHERE f.path = :file_path

    UNION ALL

    SELECT
        f2.id,
        f2.path,
        it.depth + 1,
        it.root_path
    FROM file_relations fr
    JOIN include_tree it ON fr.source_file_id = it.id
    JOIN files f2 ON fr.target_file_id = f2.id
    WHERE fr.relation_type = 'includes'
    AND it.depth < :max_depth
)
SELECT id, path, depth FROM include_tree
WHERE depth > 0
ORDER BY depth, path;
"""

# Requête pour les utilisateurs d'un type
SQL_TYPE_USERS = """
SELECT DISTINCT
    s.id,
    s.name,
    s.kind,
    s.signature,
    f.path as file_path,
    f.is_critical,
    r.relation_type,
    r.location_line
FROM symbols s
JOIN relations r ON r.source_id = s.id
JOIN files f ON s.file_id = f.id
WHERE r.target_id = :type_symbol_id
AND r.relation_type IN ('uses_type', 'returns_type', 'has_param_type', 'instantiates', 'inherits')
ORDER BY f.path, s.name;
"""

# Requête pour trouver un symbole par nom
SQL_FIND_SYMBOL_BY_NAME = """
SELECT s.*, f.path as file_path
FROM symbols s
JOIN files f ON s.file_id = f.id
WHERE s.name = :name
ORDER BY f.path;
"""

# Requête pour trouver un symbole par nom et fichier
SQL_FIND_SYMBOL_BY_NAME_AND_FILE = """
SELECT s.*, f.path as file_path
FROM symbols s
JOIN files f ON s.file_id = f.id
WHERE s.name = :name
AND f.path = :file_path;
"""

# Requête pour trouver un symbole par ID
SQL_GET_SYMBOL_BY_ID = """
SELECT s.*, f.path as file_path
FROM symbols s
JOIN files f ON s.file_id = f.id
WHERE s.id = :symbol_id;
"""


# =============================================================================
# MAIN FUNCTIONS
# =============================================================================

@_timed_query
def get_symbol_callers(
    db: Database,
    symbol_id: int,
    max_depth: int = 3,
    include_indirect: bool = True
) -> dict[str, Any]:
    """
    Trouve tous les appelants d'un symbole (récursif).

    Traverse le graphe de dépendances vers le haut pour trouver toutes
    les fonctions qui appellent directement ou indirectement le symbole donné.

    Args:
        db: Instance de Database connectée
        symbol_id: ID du symbole cible
        max_depth: Profondeur maximale de traversée (1-10, défaut: 3)
        include_indirect: Inclure les appels indirects via pointeurs

    Returns:
        Dict contenant:
        - symbol: Info sur le symbole cible {id, name, file, kind}
        - callers: Dict par niveau {level_1: [...], level_2: [...], ...}
        - summary: {total_callers, max_depth_reached, critical_callers, files_affected}

    Raises:
        ValueError: Si symbol_id n'existe pas ou max_depth invalide

    Example:
        >>> result = get_symbol_callers(db, symbol_id=42, max_depth=3)
        >>> print(result)
        {
            "symbol": {"id": 42, "name": "lcd_init", "file": "src/lcd/init.c", "kind": "function"},
            "callers": {
                "level_1": [
                    {"id": 10, "name": "system_init", "file": "src/system.c", "line": 45, "is_direct": True}
                ],
                "level_2": [
                    {"id": 5, "name": "main", "file": "src/main.c", "line": 23, "is_direct": True}
                ],
                "level_3": []
            },
            "summary": {
                "total_callers": 2,
                "max_depth_reached": 2,
                "critical_callers": 1,
                "files_affected": ["src/system.c", "src/main.c"]
            }
        }
    """
    # Validation
    if max_depth < 1 or max_depth > 10:
        raise ValueError(f"max_depth must be between 1 and 10, got {max_depth}")

    # Récupérer le symbole cible
    symbol_row = db.fetch_one(SQL_GET_SYMBOL_BY_ID, {"symbol_id": symbol_id})
    if not symbol_row:
        raise ValueError(f"Symbol with id {symbol_id} not found")

    symbol_info = {
        "id": symbol_row["id"],
        "name": symbol_row["name"],
        "file": symbol_row["file_path"],
        "kind": symbol_row["kind"],
    }

    # Exécuter la requête récursive
    rows = db.fetch_all(SQL_GET_CALLERS, {"symbol_id": symbol_id, "max_depth": max_depth})

    # Filtrer les appels indirects si demandé
    if not include_indirect:
        rows = [r for r in rows if r.get("is_direct", True)]

    # Organiser par niveau
    callers_by_level: dict[str, list[dict[str, Any]]] = {}
    for i in range(1, max_depth + 1):
        callers_by_level[f"level_{i}"] = []

    files_affected: set[str] = set()
    critical_count = 0
    max_depth_reached = 0

    for row in rows:
        depth = row["depth"]
        if depth > max_depth_reached:
            max_depth_reached = depth

        caller_info = {
            "id": row["id"],
            "name": row["name"],
            "kind": row["kind"],
            "file": row["file_path"],
            "line": row["location_line"],
            "is_direct": bool(row.get("is_direct", True)),
            "is_critical": bool(row.get("is_critical", False)),
        }

        level_key = f"level_{depth}"
        if level_key in callers_by_level:
            callers_by_level[level_key].append(caller_info)

        files_affected.add(row["file_path"])
        if row.get("is_critical"):
            critical_count += 1

    # Calculer le résumé
    total_callers = sum(len(callers) for callers in callers_by_level.values())

    return {
        "symbol": symbol_info,
        "callers": callers_by_level,
        "summary": {
            "total_callers": total_callers,
            "max_depth_reached": max_depth_reached,
            "critical_callers": critical_count,
            "files_affected": sorted(files_affected),
        },
    }


@_timed_query
def get_symbol_callees(
    db: Database,
    symbol_id: int,
    max_depth: int = 2
) -> dict[str, Any]:
    """
    Trouve tous les symboles appelés par un symbole (récursif).

    Traverse le graphe de dépendances vers le bas pour comprendre
    les dépendances d'une fonction.

    Args:
        db: Instance de Database connectée
        symbol_id: ID du symbole source
        max_depth: Profondeur maximale (1-10, défaut: 2)

    Returns:
        Dict contenant:
        - symbol: Info sur le symbole source
        - callees: Dict par niveau {level_1: [...], level_2: [...]}
        - types_used: Liste des types utilisés par ce symbole
        - summary: {total_callees, max_depth_reached, external_calls, files_affected}

    Raises:
        ValueError: Si symbol_id n'existe pas ou max_depth invalide

    Example:
        >>> result = get_symbol_callees(db, symbol_id=42, max_depth=2)
        >>> print(result["callees"]["level_1"])
        [
            {"id": 100, "name": "alloc_buffer", "file": "src/memory.c", "kind": "function"},
            {"id": 101, "name": "configure_pins", "file": "src/gpio.c", "kind": "function"}
        ]
    """
    # Validation
    if max_depth < 1 or max_depth > 10:
        raise ValueError(f"max_depth must be between 1 and 10, got {max_depth}")

    # Récupérer le symbole source
    symbol_row = db.fetch_one(SQL_GET_SYMBOL_BY_ID, {"symbol_id": symbol_id})
    if not symbol_row:
        raise ValueError(f"Symbol with id {symbol_id} not found")

    symbol_info = {
        "id": symbol_row["id"],
        "name": symbol_row["name"],
        "file": symbol_row["file_path"],
        "kind": symbol_row["kind"],
    }

    # Exécuter la requête récursive pour les callees
    rows = db.fetch_all(SQL_GET_CALLEES, {"symbol_id": symbol_id, "max_depth": max_depth})

    # Organiser par niveau
    callees_by_level: dict[str, list[dict[str, Any]]] = {}
    for i in range(1, max_depth + 1):
        callees_by_level[f"level_{i}"] = []

    files_affected: set[str] = set()
    max_depth_reached = 0
    external_count = 0

    for row in rows:
        depth = row["depth"]
        if depth > max_depth_reached:
            max_depth_reached = depth

        callee_info = {
            "id": row["id"],
            "name": row["name"],
            "kind": row["kind"],
            "file": row["file_path"],
            "line": row["location_line"],
            "is_critical": bool(row.get("is_critical", False)),
        }

        level_key = f"level_{depth}"
        if level_key in callees_by_level:
            callees_by_level[level_key].append(callee_info)

        # Vérifier si c'est un appel externe (stdlib, etc.)
        if row["file_path"] and row["file_path"].startswith(("stdlib", "libc", "/usr")):
            external_count += 1
        else:
            files_affected.add(row["file_path"])

    # Récupérer les types utilisés
    types_rows = db.fetch_all(SQL_GET_TYPES_USED, {"symbol_id": symbol_id})
    types_used = [
        {
            "id": r["id"],
            "name": r["name"],
            "kind": r["kind"],
            "file": r["file_path"],
            "relation": r["relation_type"],
        }
        for r in types_rows
    ]

    # Calculer le résumé
    total_callees = sum(len(callees) for callees in callees_by_level.values())

    return {
        "symbol": symbol_info,
        "callees": callees_by_level,
        "types_used": types_used,
        "summary": {
            "total_callees": total_callees,
            "max_depth_reached": max_depth_reached,
            "external_calls": external_count,
            "files_affected": sorted(files_affected),
        },
    }


@_timed_query
def get_file_impact(
    db: Database,
    file_path: str,
    include_transitive: bool = True,
    max_depth: int = 3
) -> dict[str, Any]:
    """
    Calcule l'impact complet de la modification d'un fichier.

    Combine deux sources d'impact :
    1. Fichiers qui incluent/importent ce fichier
    2. Fichiers dont les symboles appellent des symboles de ce fichier

    Args:
        db: Instance de Database connectée
        file_path: Chemin du fichier (relatif à la racine)
        include_transitive: Inclure les impacts de niveau 2+ (défaut: True)
        max_depth: Profondeur max pour le calcul transitif (défaut: 3)

    Returns:
        Dict contenant:
        - file: Chemin du fichier analysé
        - direct_impact: Liste des fichiers directement impactés
        - transitive_impact: Liste des fichiers indirectement impactés
        - include_impact: Liste des fichiers qui incluent ce fichier
        - summary: Stats {total_files_impacted, critical_files_impacted, max_depth}

    Raises:
        ValueError: Si file_path n'existe pas dans la base

    Example:
        >>> impact = get_file_impact(db, "src/lcd/init.c")
        >>> print(impact)
        {
            "file": "src/lcd/init.c",
            "direct_impact": [
                {"file": "src/main.c", "reason": "calls lcd_init", "symbols": ["main"]}
            ],
            "transitive_impact": [
                {"file": "src/boot.c", "reason": "calls system_init", "depth": 2}
            ],
            "include_impact": [
                {"file": "src/test/test_lcd.c", "reason": "includes lcd.h"}
            ],
            "summary": {
                "total_files_impacted": 3,
                "critical_files_impacted": 1,
                "max_depth": 2
            }
        }
    """
    # Vérifier que le fichier existe
    file_row = db.fetch_one(
        "SELECT id, path FROM files WHERE path = :path",
        {"path": file_path}
    )
    if not file_row:
        raise ValueError(f"File '{file_path}' not found in database")

    file_id = file_row["id"]

    # 1. Impact par includes/imports
    include_rows = db.fetch_all(SQL_FILE_IMPACT_BY_INCLUDES, {"file_path": file_path})
    include_impact = [
        {
            "file": r["path"],
            "reason": f"includes {file_path}",
            "line": r["line_number"],
            "is_critical": bool(r.get("is_critical", False)),
        }
        for r in include_rows
    ]

    # 2. Impact par calls (direct)
    call_rows = db.fetch_all(SQL_FILE_IMPACT_BY_CALLS, {"file_path": file_path})

    # Grouper par fichier et agréger les symboles
    direct_by_file: dict[str, dict[str, Any]] = {}
    for r in call_rows:
        path = r["path"]
        if path not in direct_by_file:
            direct_by_file[path] = {
                "file": path,
                "reason": f"calls {r['called_symbol']}",
                "symbols": [],
                "is_critical": bool(r.get("is_critical", False)),
            }
        if r["symbol_name"] not in direct_by_file[path]["symbols"]:
            direct_by_file[path]["symbols"].append(r["symbol_name"])

    direct_impact = list(direct_by_file.values())

    # 3. Impact transitif (si demandé)
    transitive_impact: list[dict[str, Any]] = []
    if include_transitive and max_depth > 1:
        # Pour chaque fichier directement impacté, chercher ses appelants
        processed_files = {file_path}
        files_to_process = [d["file"] for d in direct_impact]

        for depth in range(2, max_depth + 1):
            next_files = []
            for f in files_to_process:
                if f in processed_files:
                    continue
                processed_files.add(f)

                # Trouver les appelants des symboles de ce fichier
                trans_rows = db.fetch_all(SQL_FILE_IMPACT_BY_CALLS, {"file_path": f})
                for r in trans_rows:
                    path = r["path"]
                    if path not in processed_files:
                        transitive_impact.append({
                            "file": path,
                            "reason": f"calls {r['called_symbol']} in {f}",
                            "depth": depth,
                            "is_critical": bool(r.get("is_critical", False)),
                        })
                        next_files.append(path)

            files_to_process = list(set(next_files))
            if not files_to_process:
                break

    # Calculer le résumé
    all_files = set()
    critical_count = 0

    for item in direct_impact + transitive_impact + include_impact:
        all_files.add(item["file"])
        if item.get("is_critical"):
            critical_count += 1

    max_depth_reached = 1
    if transitive_impact:
        max_depth_reached = max(t.get("depth", 1) for t in transitive_impact)

    return {
        "file": file_path,
        "direct_impact": direct_impact,
        "transitive_impact": transitive_impact,
        "include_impact": include_impact,
        "summary": {
            "total_files_impacted": len(all_files),
            "critical_files_impacted": critical_count,
            "max_depth": max_depth_reached,
        },
    }


@_timed_query
def get_type_users(
    db: Database,
    type_symbol_id: int
) -> list[dict[str, Any]]:
    """
    Trouve toutes les fonctions/symboles qui utilisent un type donné.

    Cherche les relations : uses_type, returns_type, has_param_type,
    instantiates, inherits.

    Args:
        db: Instance de Database connectée
        type_symbol_id: ID du symbole de type (struct, class, enum, typedef)

    Returns:
        Liste de dict, chacun contenant:
        - id: ID du symbole utilisateur
        - name: Nom du symbole
        - kind: Type du symbole (function, struct, etc.)
        - signature: Signature si disponible
        - file: Chemin du fichier
        - relation_type: Type de relation (uses_type, returns_type, etc.)
        - line: Ligne de la relation
        - is_critical: Si le fichier est critique

    Raises:
        ValueError: Si type_symbol_id n'existe pas

    Example:
        >>> users = get_type_users(db, type_symbol_id=100)  # LCD_Config
        >>> for u in users:
        ...     print(f"{u['name']} ({u['relation_type']}) in {u['file']}")
        lcd_init (has_param_type) in src/lcd/init.c
        lcd_write (uses_type) in src/lcd/write.c
    """
    # Vérifier que le symbole existe et est un type
    symbol_row = db.fetch_one(SQL_GET_SYMBOL_BY_ID, {"symbol_id": type_symbol_id})
    if not symbol_row:
        raise ValueError(f"Symbol with id {type_symbol_id} not found")

    kind = symbol_row.get("kind", "")
    if kind not in ("struct", "class", "enum", "typedef", "interface", "union"):
        logger.warning(
            f"Symbol {type_symbol_id} ({symbol_row.get('name')}) is a {kind}, "
            "not a type. Results may be unexpected."
        )

    # Exécuter la requête
    rows = db.fetch_all(SQL_TYPE_USERS, {"type_symbol_id": type_symbol_id})

    return [
        {
            "id": r["id"],
            "name": r["name"],
            "kind": r["kind"],
            "signature": r.get("signature"),
            "file": r["file_path"],
            "relation_type": r["relation_type"],
            "line": r["location_line"],
            "is_critical": bool(r.get("is_critical", False)),
        }
        for r in rows
    ]


@_timed_query
def get_include_tree(
    db: Database,
    file_path: str,
    max_depth: int = 3
) -> dict[str, Any]:
    """
    Récupère l'arbre d'inclusion d'un fichier.

    Traverse récursivement les #include/#import pour construire
    l'arbre complet des dépendances d'inclusion.

    Args:
        db: Instance de Database connectée
        file_path: Chemin du fichier racine
        max_depth: Profondeur maximale (défaut: 3)

    Returns:
        Dict contenant:
        - root: Chemin du fichier racine
        - includes: Liste des fichiers inclus avec leur profondeur
        - tree: Structure arborescente {path: {includes: [...]}}
        - summary: {total_includes, max_depth_reached, unique_files}

    Raises:
        ValueError: Si file_path n'existe pas

    Example:
        >>> tree = get_include_tree(db, "src/lcd/init.c", max_depth=3)
        >>> print(tree["includes"])
        [
            {"path": "src/lcd/lcd.h", "depth": 1},
            {"path": "src/lcd/types.h", "depth": 2},
            {"path": "src/hardware/gpio.h", "depth": 1}
        ]
    """
    # Vérifier que le fichier existe
    file_row = db.fetch_one(
        "SELECT id, path FROM files WHERE path = :path",
        {"path": file_path}
    )
    if not file_row:
        raise ValueError(f"File '{file_path}' not found in database")

    # Exécuter la requête récursive
    rows = db.fetch_all(SQL_INCLUDE_TREE, {"file_path": file_path, "max_depth": max_depth})

    # Construire la liste plate des includes
    includes = [
        {"path": r["path"], "depth": r["depth"]}
        for r in rows
    ]

    # Construire l'arbre hiérarchique
    tree: dict[str, Any] = {file_path: {"includes": []}}
    by_depth: dict[int, list[str]] = {0: [file_path]}

    for r in rows:
        path = r["path"]
        depth = r["depth"]

        if depth not in by_depth:
            by_depth[depth] = []
        by_depth[depth].append(path)

        tree[path] = {"includes": []}

    # Relier les niveaux
    # Pour chaque fichier, trouver ses includes directs
    for path in tree:
        direct_includes = db.fetch_all(
            """
            SELECT f2.path
            FROM file_relations fr
            JOIN files f1 ON fr.source_file_id = f1.id
            JOIN files f2 ON fr.target_file_id = f2.id
            WHERE f1.path = :path
            AND fr.relation_type = 'includes'
            """,
            {"path": path}
        )
        tree[path]["includes"] = [r["path"] for r in direct_includes if r["path"] in tree]

    # Calculer le résumé
    unique_files = set(r["path"] for r in rows)
    max_depth_reached = max((r["depth"] for r in rows), default=0)

    return {
        "root": file_path,
        "includes": includes,
        "tree": tree,
        "summary": {
            "total_includes": len(rows),
            "max_depth_reached": max_depth_reached,
            "unique_files": len(unique_files),
        },
    }


@_timed_query
def get_symbol_by_name_qualified(
    db: Database,
    name: str,
    file_path: Optional[str] = None
) -> Optional[Symbol]:
    """
    Recherche un symbole par nom, avec désambiguïsation par fichier si nécessaire.

    Si plusieurs symboles portent le même nom et qu'aucun file_path n'est fourni,
    retourne le premier trouvé et log un warning.

    Args:
        db: Instance de Database connectée
        name: Nom du symbole à chercher
        file_path: Chemin du fichier pour désambiguïser (optionnel)

    Returns:
        Instance de Symbol si trouvé, None sinon

    Raises:
        ValueError: Si name est vide

    Example:
        >>> # Recherche simple
        >>> symbol = get_symbol_by_name_qualified(db, "lcd_init")
        >>> print(f"Found: {symbol.name} in {symbol.file_id}")

        >>> # Recherche avec désambiguïsation
        >>> symbol = get_symbol_by_name_qualified(db, "init", file_path="src/lcd/init.c")
        >>> print(f"Found specific: {symbol.signature}")
    """
    if not name or not name.strip():
        raise ValueError("Symbol name cannot be empty")

    name = name.strip()

    if file_path:
        # Recherche avec fichier spécifique
        row = db.fetch_one(SQL_FIND_SYMBOL_BY_NAME_AND_FILE, {"name": name, "file_path": file_path})
        if row:
            return Symbol.from_row(row)
        return None

    # Recherche par nom seul
    rows = db.fetch_all(SQL_FIND_SYMBOL_BY_NAME, {"name": name})

    if not rows:
        return None

    if len(rows) > 1:
        files = [r["file_path"] for r in rows]
        logger.warning(
            f"Multiple symbols found for name '{name}' in files: {files}. "
            "Returning first match. Use file_path parameter to disambiguate."
        )

    return Symbol.from_row(rows[0])


# =============================================================================
# CONVENIENCE WRAPPERS (pour compatibilité avec l'ancienne API)
# =============================================================================

def get_symbol_callers_by_name(
    db: Database,
    symbol_name: str,
    file_path: Optional[str] = None,
    max_depth: int = 3,
    include_indirect: bool = True
) -> dict[str, Any]:
    """
    Wrapper pour get_symbol_callers qui accepte un nom au lieu d'un ID.

    Args:
        db: Instance de Database
        symbol_name: Nom du symbole
        file_path: Fichier pour désambiguïser
        max_depth: Profondeur max
        include_indirect: Inclure les appels indirects

    Returns:
        Même structure que get_symbol_callers

    Raises:
        ValueError: Si le symbole n'est pas trouvé
    """
    symbol = get_symbol_by_name_qualified(db, symbol_name, file_path)
    if not symbol or not symbol.id:
        raise ValueError(f"Symbol '{symbol_name}' not found" + (f" in {file_path}" if file_path else ""))

    return get_symbol_callers(db, symbol.id, max_depth, include_indirect)


def get_symbol_callees_by_name(
    db: Database,
    symbol_name: str,
    file_path: Optional[str] = None,
    max_depth: int = 2
) -> dict[str, Any]:
    """
    Wrapper pour get_symbol_callees qui accepte un nom au lieu d'un ID.

    Args:
        db: Instance de Database
        symbol_name: Nom du symbole
        file_path: Fichier pour désambiguïser
        max_depth: Profondeur max

    Returns:
        Même structure que get_symbol_callees

    Raises:
        ValueError: Si le symbole n'est pas trouvé
    """
    symbol = get_symbol_by_name_qualified(db, symbol_name, file_path)
    if not symbol or not symbol.id:
        raise ValueError(f"Symbol '{symbol_name}' not found" + (f" in {file_path}" if file_path else ""))

    return get_symbol_callees(db, symbol.id, max_depth)


def get_type_users_by_name(
    db: Database,
    type_name: str,
    file_path: Optional[str] = None
) -> list[dict[str, Any]]:
    """
    Wrapper pour get_type_users qui accepte un nom au lieu d'un ID.

    Args:
        db: Instance de Database
        type_name: Nom du type
        file_path: Fichier pour désambiguïser

    Returns:
        Même structure que get_type_users

    Raises:
        ValueError: Si le type n'est pas trouvé
    """
    symbol = get_symbol_by_name_qualified(db, type_name, file_path)
    if not symbol or not symbol.id:
        raise ValueError(f"Type '{type_name}' not found" + (f" in {file_path}" if file_path else ""))

    return get_type_users(db, symbol.id)


# =============================================================================
# EXPORT
# =============================================================================

__all__ = [
    # Fonctions principales
    "get_symbol_callers",
    "get_symbol_callees",
    "get_file_impact",
    "get_type_users",
    "get_include_tree",
    "get_symbol_by_name_qualified",
    # Wrappers par nom
    "get_symbol_callers_by_name",
    "get_symbol_callees_by_name",
    "get_type_users_by_name",
]
