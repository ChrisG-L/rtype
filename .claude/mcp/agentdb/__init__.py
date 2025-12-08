"""
AgentDB MCP Server - Serveur Model Context Protocol pour AgentDB.

Ce module expose les fonctionnalités d'AgentDB aux agents Claude
via le protocole MCP (Model Context Protocol).

Le serveur communique via stdio en JSON-RPC 2.0 et expose 10 outils :
1. get_file_context - Contexte complet d'un fichier
2. get_symbol_callers - Appelants récursifs d'un symbole
3. get_symbol_callees - Appelés récursifs d'un symbole
4. get_file_impact - Impact de la modification d'un fichier
5. get_error_history - Historique des erreurs
6. get_patterns - Patterns applicables
7. get_architecture_decisions - ADRs applicables
8. search_symbols - Recherche de symboles
9. get_file_metrics - Métriques d'un fichier
10. get_module_summary - Résumé d'un module

Usage:
    # Lancer le serveur
    python -m agentdb.mcp_server

    # Configuration dans .claude/settings.json
    {
        "mcpServers": {
            "agentdb": {
                "command": "python",
                "args": ["-m", "mcp.agentdb.server"]
            }
        }
    }
"""

__version__ = "1.0.0"

from .server import AgentDBServer, main
from .tools import (
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

__all__ = [
    "AgentDBServer",
    "main",
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
