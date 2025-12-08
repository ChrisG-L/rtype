"""
AgentDB - Base de données contextuelle pour le système multi-agents.

Ce module fournit la mémoire partagée pour tous les agents Claude.
Il stocke et expose :
- Le graphe de dépendances (fichiers, symboles, relations)
- La mémoire historique (erreurs, runs du pipeline)
- La base de connaissances (patterns, ADRs)
- Les métriques (complexité, activité, qualité)

Usage:
    from agentdb import Database, FileRepository, SymbolRepository

    db = Database(".claude/agentdb/db.sqlite")
    files = FileRepository(db)
    symbols = SymbolRepository(db)
"""

__version__ = "2.0.0"
__author__ = "Multi-Agent System"

from .db import Database
from .models import (
    File,
    Symbol,
    Relation,
    FileRelation,
    ErrorHistory,
    PipelineRun,
    Pattern,
    ArchitectureDecision,
    CriticalPath,
)
from .crud import (
    FileRepository,
    SymbolRepository,
    RelationRepository,
    ErrorHistoryRepository,
    PipelineRunRepository,
    PatternRepository,
)
from .queries import (
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
from .indexer import CodeIndexer

__all__ = [
    # Database
    "Database",
    # Models
    "File",
    "Symbol",
    "Relation",
    "FileRelation",
    "ErrorHistory",
    "PipelineRun",
    "Pattern",
    "ArchitectureDecision",
    "CriticalPath",
    # Repositories
    "FileRepository",
    "SymbolRepository",
    "RelationRepository",
    "ErrorHistoryRepository",
    "PipelineRunRepository",
    "PatternRepository",
    # Query functions
    "get_symbol_callers",
    "get_symbol_callees",
    "get_file_impact",
    "get_type_users",
    "get_include_tree",
    "get_symbol_by_name_qualified",
    "get_symbol_callers_by_name",
    "get_symbol_callees_by_name",
    "get_type_users_by_name",
    # Indexer
    "CodeIndexer",
]
