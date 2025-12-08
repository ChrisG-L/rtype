"""
AgentDB MCP Tools - Implémentation des 10 outils MCP.

Ce module contient l'implémentation métier de chaque outil exposé
par le serveur MCP. Chaque fonction prend les arguments validés
et retourne un dict JSON-serializable.

Les outils sont groupés par usage :
- Graphe : get_file_context, get_symbol_callers/callees, get_file_impact
- Historique : get_error_history
- Connaissance : get_patterns, get_architecture_decisions
- Recherche : search_symbols
- Métriques : get_file_metrics, get_module_summary

Format de sortie conforme à AGENTDB.md PARTIE 7.2.
"""

from __future__ import annotations

import fnmatch
import logging
from datetime import datetime, timedelta
from typing import Any, Optional

logger = logging.getLogger("agentdb.mcp.tools")


# =============================================================================
# OUTIL 1 : GET_FILE_CONTEXT
# =============================================================================

def get_file_context(
    db,
    path: str,
    include_symbols: bool = True,
    include_dependencies: bool = True,
    include_history: bool = True,
    include_patterns: bool = True
) -> dict[str, Any]:
    """
    Récupère le contexte complet d'un fichier.

    C'est l'outil le plus utilisé - il donne une vue 360° :
    - Métadonnées du fichier avec métriques
    - Liste des symboles (fonctions, types, etc.)
    - Dépendances (includes, appelants/appelés)
    - Historique des erreurs
    - Patterns et ADRs applicables

    Args:
        db: Connexion à la base (Database)
        path: Chemin du fichier relatif à la racine du projet
        include_symbols: Inclure les symboles
        include_dependencies: Inclure les dépendances
        include_history: Inclure l'historique
        include_patterns: Inclure les patterns

    Returns:
        Dict avec file, symbols, dependencies, error_history, patterns,
        architecture_decisions - format conforme à PARTIE 7.2
    """
    from agentdb.crud import (
        FileRepository,
        SymbolRepository,
        FileRelationRepository,
        PatternRepository,
        ArchitectureDecisionRepository,
    )

    files_repo = FileRepository(db)
    symbols_repo = SymbolRepository(db)

    # Récupérer le fichier
    file_obj = files_repo.find_by_path(path) if hasattr(files_repo, 'find_by_path') else files_repo.get_by_path(path)
    if not file_obj:
        return {"error": f"File not found: {path}"}

    # Construire la section 'file' avec le format exact de la spec
    result: dict[str, Any] = {
        "file": {
            "path": file_obj.path,
            "module": file_obj.module,
            "language": file_obj.language,
            "is_critical": bool(file_obj.is_critical),
            "security_sensitive": bool(file_obj.security_sensitive),
            "metrics": {
                "lines_total": file_obj.lines_total or 0,
                "lines_code": file_obj.lines_code or 0,
                "lines_comment": file_obj.lines_comment or 0,
                "complexity_avg": file_obj.complexity_avg or 0.0,
                "complexity_max": file_obj.complexity_max or 0,
            },
            "activity": {
                "commits_30d": file_obj.commits_30d or 0,
                "commits_90d": file_obj.commits_90d or 0,
                "last_modified": file_obj.last_modified,
                "contributors": _get_file_contributors(db, file_obj.id),
            },
        },
    }

    # Symboles avec format de la spec
    if include_symbols:
        symbols = symbols_repo.get_by_file(file_obj.id)
        result["symbols"] = [
            {
                "name": s.name,
                "kind": s.kind,
                "signature": s.signature,
                "complexity": getattr(s, 'complexity', None),
                "has_doc": bool(getattr(s, 'doc_comment', None)),
                "line_start": s.line_start,
                "line_end": s.line_end,
            }
            for s in symbols
        ]

    # Dépendances avec format de la spec
    if include_dependencies:
        result["dependencies"] = _get_file_dependencies(db, file_obj)

    # Historique des erreurs avec format de la spec
    if include_history:
        error_result = get_error_history(db, file_path=path, days=365, limit=10)
        result["error_history"] = [
            {
                "type": e.get("type"),
                "severity": e.get("severity"),
                "title": e.get("title"),
                "resolved_at": e.get("resolved_at"),
                "resolution": e.get("resolution"),
            }
            for e in error_result.get("errors", [])
        ]

    # Patterns applicables avec format de la spec
    if include_patterns:
        patterns_repo = PatternRepository(db)
        patterns = patterns_repo.get_for_file(path)
        result["patterns"] = [
            {
                "name": p.name,
                "title": getattr(p, 'title', p.name),
                "description": p.description,
            }
            for p in patterns
        ]

        # ADRs applicables
        adr_repo = ArchitectureDecisionRepository(db)
        adrs = adr_repo.get_for_file(path)
        result["architecture_decisions"] = [
            {
                "id": adr.decision_id,
                "title": adr.title,
            }
            for adr in adrs
        ]

    return result


def _get_file_contributors(db, file_id: int) -> list[str]:
    """Récupère les contributeurs d'un fichier depuis l'historique git."""
    try:
        rows = db.fetch_all(
            """
            SELECT DISTINCT author
            FROM git_history
            WHERE file_id = ?
            ORDER BY commit_date DESC
            LIMIT 10
            """,
            (file_id,),
        )
        return [r["author"] for r in rows if r.get("author")]
    except Exception:
        # Table git_history peut ne pas exister
        return []


def _get_file_dependencies(db, file_obj) -> dict[str, Any]:
    """Extrait les dépendances d'un fichier."""
    from agentdb.crud import FileRelationRepository, SymbolRepository, RelationRepository

    deps: dict[str, Any] = {
        "includes": [],
        "included_by": [],
        "calls_to": [],
        "called_by": [],
    }

    try:
        file_rel_repo = FileRelationRepository(db)

        # Fichiers inclus par ce fichier
        includes = file_rel_repo.get_includes(file_obj.id)
        for rel in includes:
            target_file = db.fetch_one(
                "SELECT path FROM files WHERE id = ?",
                (rel.target_file_id,),
            )
            if target_file:
                deps["includes"].append(target_file["path"])

        # Fichiers qui incluent ce fichier
        included_by = file_rel_repo.get_included_by(file_obj.id)
        for rel in included_by:
            source_file = db.fetch_one(
                "SELECT path FROM files WHERE id = ?",
                (rel.source_file_id,),
            )
            if source_file:
                deps["included_by"].append(source_file["path"])

        # Fonctions appelées par les symboles de ce fichier
        symbols_repo = SymbolRepository(db)
        rel_repo = RelationRepository(db)
        symbols = symbols_repo.get_by_file(file_obj.id)

        called_symbols = set()
        caller_symbols = set()

        for sym in symbols:
            # Callees: symboles appelés par ce symbole
            callees = rel_repo.get_callees(sym.id)
            for rel in callees:
                target = db.fetch_one(
                    "SELECT name FROM symbols WHERE id = ?",
                    (rel.target_id,),
                )
                if target:
                    called_symbols.add(target["name"])

            # Callers: symboles qui appellent ce symbole
            callers = rel_repo.get_callers(sym.id)
            for rel in callers:
                source = db.fetch_one(
                    "SELECT name FROM symbols WHERE id = ?",
                    (rel.source_id,),
                )
                if source:
                    caller_symbols.add(source["name"])

        deps["calls_to"] = sorted(called_symbols)
        deps["called_by"] = sorted(caller_symbols)

    except Exception as e:
        logger.warning(f"Could not get dependencies for {file_obj.path}: {e}")

    return deps


# =============================================================================
# OUTIL 2 : GET_SYMBOL_CALLERS
# =============================================================================

def get_symbol_callers(
    db,
    symbol_name: str,
    file_path: Optional[str] = None,
    max_depth: int = 3,
    include_indirect: bool = True
) -> dict[str, Any]:
    """
    Trouve tous les appelants d'un symbole (récursif).

    Essentiel pour l'analyse d'impact : "Si je modifie cette fonction,
    qu'est-ce qui peut casser ?"

    Args:
        db: Connexion à la base
        symbol_name: Nom du symbole (fonction, variable, etc.)
        file_path: Fichier du symbole (pour désambiguïser)
        max_depth: Profondeur maximale de traversée (1-10)
        include_indirect: Inclure les appels indirects (via pointeurs)

    Returns:
        Dict avec symbol, callers (level_1, level_2, level_3), summary
        Format conforme à PARTIE 7.2
    """
    from agentdb.queries import get_symbol_callers_by_name

    try:
        raw_result = get_symbol_callers_by_name(
            db,
            symbol_name=symbol_name,
            file_path=file_path,
            max_depth=max_depth,
            include_indirect=include_indirect,
        )

        # Reformater selon la spec PARTIE 7.2
        symbol_info = raw_result.get("symbol", {})
        callers_raw = raw_result.get("callers", {})
        summary_raw = raw_result.get("summary", {})

        # Construire les niveaux avec le format exact
        callers: dict[str, list[dict[str, Any]]] = {}
        for i in range(1, max_depth + 1):
            level_key = f"level_{i}"
            level_data = callers_raw.get(level_key, [])
            callers[level_key] = [
                {
                    "name": c.get("name"),
                    "file": c.get("file"),
                    "line": c.get("line"),
                    "is_direct": c.get("is_direct", True),
                }
                for c in level_data
            ]

        return {
            "symbol": {
                "name": symbol_info.get("name"),
                "file": symbol_info.get("file"),
                "kind": symbol_info.get("kind"),
            },
            "callers": callers,
            "summary": {
                "total_callers": summary_raw.get("total_callers", 0),
                "max_depth_reached": summary_raw.get("max_depth_reached", 0),
                "critical_callers": summary_raw.get("critical_callers", 0),
                "files_affected": summary_raw.get("files_affected", []),
            },
        }

    except ValueError as e:
        return {"error": str(e)}
    except Exception as e:
        logger.error(f"Error in get_symbol_callers: {e}")
        return {"error": f"Internal error: {e}"}


# =============================================================================
# OUTIL 3 : GET_SYMBOL_CALLEES
# =============================================================================

def get_symbol_callees(
    db,
    symbol_name: str,
    file_path: Optional[str] = None,
    max_depth: int = 2
) -> dict[str, Any]:
    """
    Trouve tous les symboles appelés par un symbole.

    Utile pour comprendre les dépendances d'une fonction.

    Args:
        db: Connexion à la base
        symbol_name: Nom du symbole
        file_path: Fichier du symbole (optionnel)
        max_depth: Profondeur de traversée

    Returns:
        Dict avec symbol, callees (level_1, level_2), types_used
        Format conforme à PARTIE 7.2
    """
    from agentdb.queries import get_symbol_callees_by_name

    try:
        raw_result = get_symbol_callees_by_name(
            db,
            symbol_name=symbol_name,
            file_path=file_path,
            max_depth=max_depth,
        )

        # Reformater selon la spec PARTIE 7.2
        symbol_info = raw_result.get("symbol", {})
        callees_raw = raw_result.get("callees", {})
        types_raw = raw_result.get("types_used", [])

        # Construire les niveaux avec le format exact
        callees: dict[str, list[dict[str, Any]]] = {}
        for i in range(1, max_depth + 1):
            level_key = f"level_{i}"
            level_data = callees_raw.get(level_key, [])
            callees[level_key] = [
                {
                    "name": c.get("name"),
                    "file": c.get("file"),
                    "kind": c.get("kind"),
                    # Marquer comme external si le fichier est externe
                    **({"external": True} if _is_external_file(c.get("file")) else {}),
                }
                for c in level_data
            ]

        # Types utilisés avec format de la spec
        types_used = [
            {
                "name": t.get("name"),
                "file": t.get("file"),
            }
            for t in types_raw
        ]

        return {
            "symbol": {
                "name": symbol_info.get("name"),
                "file": symbol_info.get("file"),
            },
            "callees": callees,
            "types_used": types_used,
        }

    except ValueError as e:
        return {"error": str(e)}
    except Exception as e:
        logger.error(f"Error in get_symbol_callees: {e}")
        return {"error": f"Internal error: {e}"}


def _is_external_file(file_path: Optional[str]) -> bool:
    """Vérifie si un fichier est externe (stdlib, libc, etc.)."""
    if not file_path:
        return False
    return file_path.startswith(("stdlib", "libc", "/usr", "<"))


# =============================================================================
# OUTIL 4 : GET_FILE_IMPACT
# =============================================================================

def get_file_impact(
    db,
    path: str,
    include_transitive: bool = True,
    max_depth: int = 3
) -> dict[str, Any]:
    """
    Calcule l'impact complet de la modification d'un fichier.

    Combine : fichiers qui incluent + fichiers avec symboles appelants.

    Args:
        db: Connexion à la base
        path: Chemin du fichier
        include_transitive: Inclure les impacts transitifs
        max_depth: Profondeur max pour le calcul transitif

    Returns:
        Dict avec file, direct_impact, transitive_impact, include_impact, summary
        Format conforme à PARTIE 7.2
    """
    from agentdb.queries import get_file_impact as query_file_impact

    try:
        raw_result = query_file_impact(
            db,
            file_path=path,
            include_transitive=include_transitive,
            max_depth=max_depth,
        )

        # Reformater selon la spec PARTIE 7.2
        direct_raw = raw_result.get("direct_impact", [])
        transitive_raw = raw_result.get("transitive_impact", [])
        include_raw = raw_result.get("include_impact", [])
        summary_raw = raw_result.get("summary", {})

        # Format exact de la spec pour direct_impact
        direct_impact = [
            {
                "file": d.get("file"),
                "reason": d.get("reason", f"calls symbol"),
                "symbols": d.get("symbols", []),
            }
            for d in direct_raw
        ]

        # Format exact pour transitive_impact
        transitive_impact = [
            {
                "file": t.get("file"),
                "reason": t.get("reason"),
                "depth": t.get("depth", 2),
            }
            for t in transitive_raw
        ]

        # Format exact pour include_impact
        include_impact = [
            {
                "file": i.get("file"),
                "reason": i.get("reason", f"includes {path}"),
            }
            for i in include_raw
        ]

        return {
            "file": path,
            "direct_impact": direct_impact,
            "transitive_impact": transitive_impact,
            "include_impact": include_impact,
            "summary": {
                "total_files_impacted": summary_raw.get("total_files_impacted", 0),
                "critical_files_impacted": summary_raw.get("critical_files_impacted", 0),
                "max_depth": summary_raw.get("max_depth", 1),
            },
        }

    except ValueError as e:
        return {"error": str(e)}
    except Exception as e:
        logger.error(f"Error in get_file_impact: {e}")
        return {"error": f"Internal error: {e}"}


# =============================================================================
# OUTIL 5 : GET_ERROR_HISTORY
# =============================================================================

def get_error_history(
    db,
    file_path: Optional[str] = None,
    symbol_name: Optional[str] = None,
    module: Optional[str] = None,
    error_type: Optional[str] = None,
    severity: Optional[str] = None,
    days: int = 180,
    limit: int = 20
) -> dict[str, Any]:
    """
    Récupère l'historique des erreurs/bugs pour un fichier, un symbole,
    ou un module entier.

    Args:
        db: Connexion à la base
        file_path: Filtrer par fichier
        symbol_name: Filtrer par symbole
        module: Filtrer par module
        error_type: Filtrer par type d'erreur
        severity: Sévérité minimum (critical, high, medium, low)
        days: Période en jours
        limit: Nombre max de résultats

    Returns:
        Dict avec query, errors, statistics
        Format conforme à PARTIE 7.2
    """
    # Construire la requête SQL dynamiquement
    query_parts = ["SELECT e.*"]
    from_parts = ["FROM error_history e"]
    where_parts = ["WHERE 1=1"]
    params: list[Any] = []

    # Jointure avec files si nécessaire
    if file_path or module:
        from_parts.append("LEFT JOIN files f ON e.file_id = f.id")

    if file_path:
        where_parts.append("AND (e.file_path = ? OR f.path = ?)")
        params.extend([file_path, file_path])

    if symbol_name:
        where_parts.append("AND e.symbol_name = ?")
        params.append(symbol_name)

    if module:
        where_parts.append("AND f.module = ?")
        params.append(module)

    if error_type:
        where_parts.append("AND e.error_type = ?")
        params.append(error_type)

    if severity:
        # Filtrer par sévérité minimum
        severity_order = ["critical", "high", "medium", "low"]
        if severity in severity_order:
            idx = severity_order.index(severity)
            allowed = severity_order[:idx + 1]  # critical et au-dessus
            placeholders = ",".join("?" * len(allowed))
            where_parts.append(f"AND e.severity IN ({placeholders})")
            params.extend(allowed)

    # Filtre temporel
    cutoff_date = (datetime.now() - timedelta(days=days)).isoformat()
    where_parts.append("AND e.discovered_at >= ?")
    params.append(cutoff_date)

    # Tri et limite
    order_by = "ORDER BY e.discovered_at DESC"
    limit_clause = f"LIMIT ?"
    params.append(limit)

    # Construire la requête finale
    sql = " ".join(query_parts + from_parts + where_parts + [order_by, limit_clause])

    try:
        rows = db.fetch_all(sql, tuple(params))

        # Format exact de la spec pour errors
        errors = [
            {
                "id": r.get("id"),
                "type": r.get("error_type"),
                "severity": r.get("severity"),
                "title": r.get("title"),
                "description": r.get("description"),
                "discovered_at": r.get("discovered_at"),
                "resolved_at": r.get("resolved_at"),
                "resolution": r.get("resolution"),
                "prevention": r.get("prevention"),
                "is_regression": bool(r.get("is_regression", False)),
                "jira_ticket": r.get("jira_ticket") or r.get("ticket_id"),
            }
            for r in rows
        ]

        # Calculer les statistiques
        by_type: dict[str, int] = {}
        by_severity: dict[str, int] = {}
        regression_count = 0

        for e in errors:
            et = e.get("type") or "unknown"
            by_type[et] = by_type.get(et, 0) + 1

            es = e.get("severity") or "unknown"
            by_severity[es] = by_severity.get(es, 0) + 1

            if e.get("is_regression"):
                regression_count += 1

        total_errors = len(errors)
        regression_rate = round(regression_count / total_errors, 2) if total_errors > 0 else 0.0

        return {
            "query": {
                "file_path": file_path,
                "symbol_name": symbol_name,
                "module": module,
                "error_type": error_type,
                "severity": severity,
                "days": days,
            },
            "errors": errors,
            "statistics": {
                "total_errors": total_errors,
                "by_type": by_type,
                "by_severity": by_severity,
                "regression_rate": regression_rate,
            },
        }

    except Exception as e:
        logger.error(f"Error in get_error_history: {e}")
        return {
            "error": f"Database error: {e}",
            "query": {"file_path": file_path, "days": days},
            "errors": [],
            "statistics": {"total_errors": 0, "by_type": {}, "by_severity": {}, "regression_rate": 0.0},
        }


# =============================================================================
# OUTIL 6 : GET_PATTERNS
# =============================================================================

def get_patterns(
    db,
    file_path: Optional[str] = None,
    module: Optional[str] = None,
    category: Optional[str] = None,
    include_examples: bool = True,
) -> dict[str, Any]:
    """
    Récupère les patterns de code applicables à un fichier ou module.

    Retourne deux listes :
    - applicable_patterns : patterns spécifiques au fichier/module
    - project_patterns : patterns globaux du projet

    Args:
        db: Connexion à la base
        file_path: Fichier pour lequel récupérer les patterns
        module: Module pour lequel récupérer les patterns
        category: Catégorie de patterns

    Returns:
        Dict avec applicable_patterns et project_patterns
        Format conforme à PARTIE 7.2
    """
    query = """
        SELECT * FROM patterns
        WHERE is_active = 1
    """
    params: list[Any] = []

    if category:
        query += " AND category = ?"
        params.append(category)

    query += " ORDER BY severity DESC, name"

    try:
        rows = db.fetch_all(query, tuple(params))

        applicable_patterns: list[dict[str, Any]] = []
        project_patterns: list[dict[str, Any]] = []

        for r in rows:
            # Construire le pattern avec le format exact de la spec
            pattern: dict[str, Any] = {
                "name": r.get("name"),
                "category": r.get("category"),
                "title": r.get("title") or r.get("name"),
                "description": r.get("description"),
                "severity": r.get("severity"),
            }

            # Exemples
            if r.get("good_example"):
                pattern["good_example"] = r.get("good_example")
            if r.get("bad_example"):
                pattern["bad_example"] = r.get("bad_example")

            # Règles (stockées en JSON)
            rules_json = r.get("rules_json")
            if rules_json:
                try:
                    import json
                    pattern["rules"] = json.loads(rules_json)
                except Exception:
                    pass

            # Déterminer le scope
            scope = r.get("scope", "project")
            pattern_module = r.get("module")
            file_pattern = r.get("file_pattern")

            # Pattern de niveau projet
            if scope == "project" or (not pattern_module and not file_pattern):
                project_patterns.append(pattern)
            else:
                # Pattern applicable au fichier/module spécifié
                is_applicable = False

                if file_path and file_pattern:
                    if fnmatch.fnmatch(file_path, file_pattern):
                        is_applicable = True

                if module and pattern_module:
                    if pattern_module == module or pattern_module == "*":
                        is_applicable = True

                # Si pas de filtre spécifié, le pattern est applicable
                if not file_path and not module:
                    is_applicable = True

                if is_applicable:
                    applicable_patterns.append(pattern)

        return {
            "applicable_patterns": applicable_patterns,
            "project_patterns": project_patterns,
        }

    except Exception as e:
        logger.error(f"Error in get_patterns: {e}")
        return {
            "error": f"Database error: {e}",
            "applicable_patterns": [],
            "project_patterns": [],
        }


# =============================================================================
# OUTIL 7 : GET_ARCHITECTURE_DECISIONS
# =============================================================================

def get_architecture_decisions(
    db,
    module: Optional[str] = None,
    file_path: Optional[str] = None,
    status: str = "accepted",
    include_superseded: bool = False,
) -> dict[str, Any]:
    """
    Récupère les décisions architecturales (ADR) applicables.

    Args:
        db: Connexion à la base
        module: Filtrer par module
        file_path: Filtrer par fichier
        status: Statut des décisions (accepted, proposed, deprecated)

    Returns:
        Dict avec decisions[]
        Format conforme à PARTIE 7.2
    """
    query = "SELECT * FROM architecture_decisions WHERE 1=1"
    params: list[Any] = []

    if status:
        if include_superseded:
            query += " AND (status = ? OR status = 'superseded')"
        else:
            query += " AND status = ?"
        params.append(status)

    if module:
        query += " AND (affected_modules_json LIKE ? OR affected_modules_json IS NULL)"
        params.append(f'%"{module}"%')

    query += " ORDER BY date_decided DESC"

    try:
        rows = db.fetch_all(query, tuple(params))
        decisions: list[dict[str, Any]] = []

        for r in rows:
            # Filtrer par file_path si spécifié
            if file_path:
                affected_files = r.get("affected_files_json", "")
                applies_to = r.get("applies_to_pattern", "")

                # Vérifier si le fichier correspond
                file_matches = False
                if affected_files and file_path in affected_files:
                    file_matches = True
                if applies_to and fnmatch.fnmatch(file_path, applies_to):
                    file_matches = True
                if not affected_files and not applies_to:
                    file_matches = True  # ADR global

                if not file_matches:
                    continue

            # Format exact de la spec PARTIE 7.2
            decisions.append({
                "id": r.get("decision_id"),
                "title": r.get("title"),
                "status": r.get("status"),
                "context": r.get("context"),
                "decision": r.get("decision"),
                "consequences": r.get("consequences"),
                "date_decided": r.get("date_decided"),
                "decided_by": r.get("decided_by") or r.get("author"),
            })

        return {
            "decisions": decisions,
        }

    except Exception as e:
        logger.error(f"Error in get_architecture_decisions: {e}")
        return {"error": f"Database error: {e}", "decisions": []}


# =============================================================================
# OUTIL 8 : SEARCH_SYMBOLS
# =============================================================================

def search_symbols(
    db,
    query: str,
    kind: Optional[str] = None,
    module: Optional[str] = None,
    file_path: Optional[str] = None,
    limit: int = 50
) -> dict[str, Any]:
    """
    Recherche des symboles par nom, type, ou pattern.

    Supporte les wildcards * et ? dans le pattern de recherche.

    Args:
        db: Connexion à la base
        query: Pattern de recherche (supporte * et ?)
        kind: Type de symbole (function, struct, class, enum, macro, variable)
        module: Filtrer par module
        limit: Nombre max de résultats

    Returns:
        Dict avec query, results[], total, returned
        Format conforme à PARTIE 7.2
    """
    # Convertir le pattern glob en LIKE SQL
    sql_pattern = query.replace("*", "%").replace("?", "_")

    # D'abord compter le total sans limite
    count_sql = """
        SELECT COUNT(*) as total
        FROM symbols s
        JOIN files f ON s.file_id = f.id
        WHERE s.name LIKE ?
    """
    count_params: list[Any] = [sql_pattern]

    if kind:
        count_sql += " AND s.kind = ?"
        count_params.append(kind)

    if module:
        count_sql += " AND f.module = ?"
        count_params.append(module)

    if file_path:
        count_sql += " AND f.path = ?"
        count_params.append(file_path)

    # Requête principale avec limite
    sql = """
        SELECT s.*, f.path as file_path, f.module
        FROM symbols s
        JOIN files f ON s.file_id = f.id
        WHERE s.name LIKE ?
    """
    params: list[Any] = [sql_pattern]

    if kind:
        sql += " AND s.kind = ?"
        params.append(kind)

    if module:
        sql += " AND f.module = ?"
        params.append(module)

    if file_path:
        sql += " AND f.path = ?"
        params.append(file_path)

    sql += " ORDER BY s.name LIMIT ?"
    params.append(limit)

    try:
        # Compter le total
        count_row = db.fetch_one(count_sql, tuple(count_params))
        total = count_row.get("total", 0) if count_row else 0

        # Récupérer les résultats
        rows = db.fetch_all(sql, tuple(params))

        # Format exact de la spec PARTIE 7.2
        results = [
            {
                "name": r.get("name"),
                "kind": r.get("kind"),
                "file": r.get("file_path"),
                "signature": r.get("signature"),
                "line": r.get("line_start"),
            }
            for r in rows
        ]

        return {
            "query": query,
            "results": results,
            "total": total,
            "returned": len(results),
        }

    except Exception as e:
        logger.error(f"Error in search_symbols: {e}")
        return {
            "error": f"Database error: {e}",
            "query": query,
            "results": [],
            "total": 0,
            "returned": 0,
        }


# =============================================================================
# OUTIL 9 : GET_FILE_METRICS
# =============================================================================

def get_file_metrics(
    db,
    path: str,
    include_per_function: bool = False,
) -> dict[str, Any]:
    """
    Récupère les métriques détaillées d'un fichier.

    Args:
        db: Connexion à la base
        path: Chemin du fichier

    Returns:
        Dict avec file, size, complexity, structure, quality, activity
        Format conforme à PARTIE 7.2
    """
    from agentdb.crud import FileRepository, SymbolRepository

    files_repo = FileRepository(db)
    file_obj = files_repo.find_by_path(path) if hasattr(files_repo, 'find_by_path') else files_repo.get_by_path(path)

    if not file_obj:
        return {"error": f"File not found: {path}"}

    # Compter les symboles par type
    symbols_repo = SymbolRepository(db)
    symbols = symbols_repo.get_by_file(file_obj.id)

    functions_count = sum(1 for s in symbols if s.kind in ("function", "method"))
    types_count = sum(1 for s in symbols if s.kind in ("struct", "class", "enum", "typedef", "union"))
    macros_count = sum(1 for s in symbols if s.kind == "macro")
    variables_count = sum(1 for s in symbols if s.kind in ("variable", "constant"))

    # Calculer l'âge du fichier
    age_days = None
    if file_obj.created_at:
        try:
            created = datetime.fromisoformat(file_obj.created_at.replace("Z", "+00:00"))
            age_days = (datetime.now(created.tzinfo) - created).days
        except Exception:
            pass

    # Récupérer les contributeurs
    contributors = _get_file_contributors(db, file_obj.id)

    # Calculer le score de documentation
    documented_symbols = sum(1 for s in symbols if getattr(s, 'doc_comment', None))
    doc_score = round((documented_symbols / len(symbols) * 100) if symbols else 0)

    # Vérifier s'il y a des tests
    has_tests = _file_has_tests(db, file_obj)

    # Calculer le score de dette technique
    tech_debt_score = _calculate_tech_debt_score(file_obj, symbols)

    # Format exact de la spec PARTIE 7.2
    return {
        "file": path,
        "size": {
            "lines_total": file_obj.lines_total or 0,
            "lines_code": file_obj.lines_code or 0,
            "lines_comment": file_obj.lines_comment or 0,
            "lines_blank": file_obj.lines_blank or 0,
        },
        "complexity": {
            "cyclomatic_total": file_obj.complexity_sum or 0,
            "cyclomatic_avg": file_obj.complexity_avg or 0.0,
            "cyclomatic_max": file_obj.complexity_max or 0,
            "cognitive_total": getattr(file_obj, 'cognitive_complexity', None) or 0,
            "nesting_max": getattr(file_obj, 'nesting_max', None) or 0,
        },
        "structure": {
            "functions": functions_count,
            "types": types_count,
            "macros": macros_count,
            "variables": variables_count,
        },
        "quality": {
            "documentation_score": doc_score,
            "has_tests": has_tests,
            "technical_debt_score": tech_debt_score,
        },
        "activity": {
            "commits_30d": file_obj.commits_30d or 0,
            "commits_90d": file_obj.commits_90d or 0,
            "commits_365d": file_obj.commits_365d or 0,
            "contributors": contributors,
            "last_modified": file_obj.last_modified,
            "age_days": age_days,
        },
    }


def _file_has_tests(db, file_obj) -> bool:
    """Vérifie si le fichier a des tests associés."""
    try:
        # Chercher un fichier de test correspondant
        test_patterns = [
            f"test_{file_obj.filename}",
            f"{file_obj.filename.replace('.c', '_test.c')}",
            f"{file_obj.filename.replace('.py', '_test.py')}",
            f"test_{file_obj.path}",
        ]

        for pattern in test_patterns:
            row = db.fetch_one(
                "SELECT id FROM files WHERE path LIKE ?",
                (f"%{pattern}%",),
            )
            if row:
                return True

        return False
    except Exception:
        return False


def _calculate_tech_debt_score(file_obj, symbols) -> int:
    """Calcule un score de dette technique (0-100, plus bas = mieux)."""
    score = 0

    # Complexité élevée
    if file_obj.complexity_max and file_obj.complexity_max > 20:
        score += 25
    elif file_obj.complexity_max and file_obj.complexity_max > 10:
        score += 10

    # Fichier trop long
    if file_obj.lines_code and file_obj.lines_code > 500:
        score += 20
    elif file_obj.lines_code and file_obj.lines_code > 300:
        score += 10

    # Trop de fonctions
    functions = [s for s in symbols if s.kind in ("function", "method")]
    if len(functions) > 20:
        score += 15
    elif len(functions) > 10:
        score += 5

    # Manque de documentation
    documented = sum(1 for s in symbols if getattr(s, 'doc_comment', None))
    if symbols and documented / len(symbols) < 0.3:
        score += 20
    elif symbols and documented / len(symbols) < 0.5:
        score += 10

    # Changements fréquents (instabilité)
    if file_obj.commits_30d and file_obj.commits_30d > 10:
        score += 10

    return min(score, 100)


# =============================================================================
# OUTIL 10 : GET_MODULE_SUMMARY
# =============================================================================

def get_module_summary(
    db,
    module: str,
    include_private: bool = False,
) -> dict[str, Any]:
    """
    Récupère un résumé complet d'un module (ensemble de fichiers).

    Args:
        db: Connexion à la base
        module: Nom du module

    Returns:
        Dict avec module, files, symbols, metrics, health, patterns, adrs, dependencies
        Format conforme à PARTIE 7.2
    """
    # Récupérer tous les fichiers du module
    files_query = "SELECT * FROM files WHERE module = ?"
    files_rows = db.fetch_all(files_query, (module,))

    if not files_rows:
        return {"error": f"Module not found: {module}"}

    file_ids = [r.get("id") for r in files_rows]

    # Catégoriser les fichiers
    sources_count = 0
    headers_count = 0
    tests_count = 0
    critical_count = 0

    for r in files_rows:
        ext = r.get("extension", "")
        path = r.get("path", "")

        if ext in (".c", ".cpp", ".py", ".js", ".ts", ".go", ".rs"):
            sources_count += 1
        elif ext in (".h", ".hpp", ".pyi"):
            headers_count += 1

        if "test" in path.lower() or r.get("file_type") == "test":
            tests_count += 1

        if r.get("is_critical"):
            critical_count += 1

    # Compter les symboles par type
    functions_count = 0
    types_count = 0
    macros_count = 0

    if file_ids:
        placeholders = ",".join("?" * len(file_ids))
        symbols_query = f"""
            SELECT kind, COUNT(*) as cnt
            FROM symbols
            WHERE file_id IN ({placeholders})
        """
        if not include_private:
            symbols_query += " AND name NOT LIKE '\\_%' ESCAPE '\\'"
        symbols_query += " GROUP BY kind"
        symbol_counts = db.fetch_all(symbols_query, tuple(file_ids))

        for row in symbol_counts:
            kind = row.get("kind", "")
            cnt = row.get("cnt", 0)

            if kind in ("function", "method"):
                functions_count += cnt
            elif kind in ("struct", "class", "enum", "typedef", "union", "interface"):
                types_count += cnt
            elif kind == "macro":
                macros_count += cnt

    # Métriques agrégées
    total_lines = sum(r.get("lines_code", 0) or 0 for r in files_rows)
    total_complexity = sum(r.get("complexity_sum", 0) or 0 for r in files_rows)
    avg_complexity = round(total_complexity / len(files_rows), 1) if files_rows else 0

    # Score de documentation agrégé
    documented_files = sum(1 for r in files_rows if r.get("documentation_score", 0) > 50)
    doc_score = round((documented_files / len(files_rows) * 100) if files_rows else 0)

    # Santé du module
    error_count = 0
    if file_ids:
        placeholders = ",".join("?" * len(file_ids))
        cutoff = (datetime.now() - timedelta(days=90)).isoformat()
        error_query = f"""
            SELECT COUNT(*) as cnt FROM error_history
            WHERE file_id IN ({placeholders}) AND discovered_at >= ?
        """
        error_row = db.fetch_one(error_query, tuple(file_ids) + (cutoff,))
        error_count = error_row.get("cnt", 0) if error_row else 0

    # Déterminer la couverture de tests et la dette technique
    test_coverage = "none"
    if tests_count > 0:
        test_coverage = "full" if tests_count >= sources_count else "partial"

    tech_debt = "low"
    if avg_complexity > 15 or error_count > 5:
        tech_debt = "high"
    elif avg_complexity > 10 or error_count > 2:
        tech_debt = "medium"

    # Patterns applicables au module
    patterns_query = """
        SELECT name FROM patterns
        WHERE is_active = 1
        AND (module = ? OR module IS NULL OR module = '*')
    """
    pattern_rows = db.fetch_all(patterns_query, (module,))
    patterns = [r.get("name") for r in pattern_rows if r.get("name")]

    # ADRs applicables au module
    adr_query = """
        SELECT decision_id FROM architecture_decisions
        WHERE status = 'accepted'
        AND affected_modules_json LIKE ?
    """
    adr_rows = db.fetch_all(adr_query, (f'%"{module}"%',))
    adrs = [r.get("decision_id") for r in adr_rows if r.get("decision_id")]

    # Dépendances inter-modules
    deps_query = """
        SELECT DISTINCT f2.module as dep_module
        FROM file_relations fr
        JOIN files f1 ON fr.source_file_id = f1.id
        JOIN files f2 ON fr.target_file_id = f2.id
        WHERE f1.module = ? AND f2.module != ? AND f2.module IS NOT NULL
    """
    uses_rows = db.fetch_all(deps_query, (module, module))
    depends_on = [r.get("dep_module") for r in uses_rows if r.get("dep_module")]

    used_by_query = """
        SELECT DISTINCT f1.module as dep_module
        FROM file_relations fr
        JOIN files f1 ON fr.source_file_id = f1.id
        JOIN files f2 ON fr.target_file_id = f2.id
        WHERE f2.module = ? AND f1.module != ? AND f1.module IS NOT NULL
    """
    used_by_rows = db.fetch_all(used_by_query, (module, module))
    depended_by = [r.get("dep_module") for r in used_by_rows if r.get("dep_module")]

    # Format exact de la spec PARTIE 7.2
    return {
        "module": module,
        "files": {
            "total": len(files_rows),
            "sources": sources_count,
            "headers": headers_count,
            "tests": tests_count,
            "critical": critical_count,
        },
        "symbols": {
            "functions": functions_count,
            "types": types_count,
            "macros": macros_count,
        },
        "metrics": {
            "lines_total": total_lines,
            "complexity_avg": avg_complexity,
            "documentation_score": doc_score,
        },
        "health": {
            "errors_last_90d": error_count,
            "test_coverage": test_coverage,
            "technical_debt": tech_debt,
        },
        "patterns": patterns,
        "adrs": adrs,
        "dependencies": {
            "depends_on": depends_on,
            "depended_by": depended_by,
        },
    }


# =============================================================================
# EXPORTS
# =============================================================================

__all__ = [
    "get_file_context",
    "get_symbol_callers",
    "get_symbol_callees",
    "get_file_impact",
    "get_error_history",
    "get_patterns",
    "get_architecture_decisions",
    "search_symbols",
    "get_file_metrics",
    "get_module_summary",
]
