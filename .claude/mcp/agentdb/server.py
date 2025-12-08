"""
AgentDB MCP Server - Serveur principal.

Ce module implémente le serveur MCP qui expose AgentDB aux agents Claude.

Architecture :
- Transport : stdio (JSON-RPC 2.0)
- Protocole : MCP (Model Context Protocol)
- Backend : SQLite via le module agentdb

Le serveur gère :
- L'initialisation de la connexion DB
- L'enregistrement des 10 outils MCP
- Le dispatch des requêtes JSON-RPC
- La gestion des erreurs

Usage:
    # Via CLI
    python -m mcp.agentdb.server

    # Programmatique
    server = AgentDBServer()
    asyncio.run(server.run())

    # Avec config personnalisée
    server = AgentDBServer(
        db_path=".claude/agentdb/db.sqlite",
        config_path=".claude/config/agentdb.yaml"
    )
"""

from __future__ import annotations

import asyncio
import json
import logging
import os
import sys
import traceback
from datetime import datetime
from pathlib import Path
from typing import Any, Callable, Optional, Union

# Ajouter le path pour les imports locaux
sys.path.insert(0, str(Path(__file__).parent.parent.parent))

# Configuration du logging
logging.basicConfig(
    level=os.environ.get("AGENTDB_LOG_LEVEL", "INFO"),
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    stream=sys.stderr  # Log sur stderr pour ne pas polluer stdout (utilisé par MCP)
)
logger = logging.getLogger("agentdb.mcp.server")


# =============================================================================
# CONSTANTS
# =============================================================================

# Version du serveur
SERVER_VERSION = "1.0.0"
SERVER_NAME = "agentdb"

# Codes d'erreur JSON-RPC
PARSE_ERROR = -32700
INVALID_REQUEST = -32600
METHOD_NOT_FOUND = -32601
INVALID_PARAMS = -32602
INTERNAL_ERROR = -32603

# Erreurs spécifiques AgentDB
TOOL_NOT_FOUND = -32000
DATABASE_ERROR = -32001
SYMBOL_NOT_FOUND = -32002
FILE_NOT_FOUND = -32003


# =============================================================================
# TOOL DEFINITIONS (10 outils MCP)
# =============================================================================

TOOL_DEFINITIONS = [
    # 1. get_file_context - Vue 360° d'un fichier
    {
        "name": "get_file_context",
        "description": """Récupère le contexte complet d'un fichier : métadonnées, symboles définis,
dépendances (includes/imports et appelants), historique d'erreurs associé,
métriques de code (complexité, lignes), et patterns applicables.

C'est l'outil principal pour comprendre un fichier avant de le modifier.
Répond à : "Qu'est-ce que ce fichier ? Qui l'utilise ? Quels problèmes passés ?"

Exemple de retour:
{
  "file": {"path": "src/lcd/init.c", "module": "lcd", "language": "c", ...},
  "symbols": [{"name": "lcd_init", "kind": "function", "line": 42, ...}],
  "dependencies": {"includes": [...], "included_by": [...], "callers": [...]},
  "error_history": [{"type": "null_pointer", "date": "2024-01-15", ...}],
  "metrics": {"complexity": 15, "lines_code": 127, ...},
  "patterns": [{"name": "error_handling", "description": "..."}]
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "description": "Chemin du fichier relatif à la racine du projet"
                },
                "include_symbols": {
                    "type": "boolean",
                    "default": True,
                    "description": "Inclure la liste des symboles définis dans le fichier"
                },
                "include_dependencies": {
                    "type": "boolean",
                    "default": True,
                    "description": "Inclure les dépendances (includes, appelants, appelés)"
                },
                "include_history": {
                    "type": "boolean",
                    "default": True,
                    "description": "Inclure l'historique des erreurs/bugs"
                },
                "include_patterns": {
                    "type": "boolean",
                    "default": True,
                    "description": "Inclure les patterns de code applicables"
                }
            },
            "required": ["path"]
        }
    },

    # 2. get_symbol_callers - Qui appelle ce symbole ?
    {
        "name": "get_symbol_callers",
        "description": """Trouve tous les symboles qui appellent le symbole donné,
avec traversée récursive jusqu'à une profondeur configurable.

Essentiel pour l'analyse d'impact : "Si je modifie cette fonction,
quels autres fichiers/fonctions seront affectés ?"

Retourne les appelants groupés par niveau (level_1 = appelants directs,
level_2 = appelants des appelants, etc.).

Exemple:
{
  "symbol": {"name": "lcd_write", "file": "src/lcd/io.c"},
  "callers": {
    "level_1": [{"name": "lcd_print", "file": "src/lcd/print.c", "line": 45}],
    "level_2": [{"name": "display_message", "file": "src/ui/display.c"}]
  },
  "summary": {"total": 15, "max_depth_reached": 2, "critical_callers": 3}
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "symbol_name": {
                    "type": "string",
                    "description": "Nom du symbole (fonction, variable globale, macro)"
                },
                "file_path": {
                    "type": "string",
                    "description": "Fichier contenant le symbole (pour désambiguïser si plusieurs symboles ont le même nom)"
                },
                "max_depth": {
                    "type": "integer",
                    "default": 3,
                    "minimum": 1,
                    "maximum": 10,
                    "description": "Profondeur maximale de traversée récursive"
                },
                "include_indirect": {
                    "type": "boolean",
                    "default": True,
                    "description": "Inclure les appels indirects (via pointeurs de fonction)"
                }
            },
            "required": ["symbol_name"]
        }
    },

    # 3. get_symbol_callees - Qu'est-ce que ce symbole appelle ?
    {
        "name": "get_symbol_callees",
        "description": """Trouve tous les symboles appelés par le symbole donné.

Utile pour comprendre les dépendances d'une fonction :
"Quelles fonctions cette fonction utilise-t-elle ?"

Inclut également les types utilisés (paramètres, retour, variables locales).

Exemple:
{
  "symbol": {"name": "lcd_init", "file": "src/lcd/init.c"},
  "callees": {
    "level_1": [
      {"name": "gpio_configure", "file": "src/hal/gpio.c"},
      {"name": "delay_ms", "file": "src/hal/time.c"}
    ]
  },
  "types_used": [
    {"name": "LCD_Config", "file": "src/lcd/types.h", "usage": "parameter"}
  ]
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "symbol_name": {
                    "type": "string",
                    "description": "Nom du symbole"
                },
                "file_path": {
                    "type": "string",
                    "description": "Fichier contenant le symbole (optionnel, pour désambiguïser)"
                },
                "max_depth": {
                    "type": "integer",
                    "default": 2,
                    "minimum": 1,
                    "maximum": 5,
                    "description": "Profondeur de traversée (généralement 1-2 suffit)"
                }
            },
            "required": ["symbol_name"]
        }
    },

    # 4. get_file_impact - Impact d'une modification de fichier
    {
        "name": "get_file_impact",
        "description": """Calcule l'impact complet de la modification d'un fichier.

Combine plusieurs sources d'impact :
1. Fichiers qui #include ce fichier (impact direct sur les headers)
2. Fichiers dont les symboles appellent des symboles de ce fichier
3. Impact transitif (fichiers impactés par les fichiers impactés)

Répond à : "Si je modifie ce fichier, quoi d'autre pourrait casser ?"

Exemple:
{
  "file": "src/lcd/types.h",
  "direct_impact": [
    {"file": "src/lcd/init.c", "reason": "includes types.h"},
    {"file": "src/lcd/write.c", "reason": "calls lcd_config_t"}
  ],
  "transitive_impact": [
    {"file": "src/main.c", "reason": "includes init.c", "depth": 2}
  ],
  "summary": {
    "total_files_impacted": 12,
    "critical_files_impacted": 2,
    "test_files_impacted": 3
  }
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "description": "Chemin du fichier à analyser"
                },
                "include_transitive": {
                    "type": "boolean",
                    "default": True,
                    "description": "Inclure les impacts de niveau 2+ (transitifs)"
                },
                "max_depth": {
                    "type": "integer",
                    "default": 3,
                    "minimum": 1,
                    "maximum": 5,
                    "description": "Profondeur max pour l'impact transitif"
                }
            },
            "required": ["path"]
        }
    },

    # 5. get_error_history - Historique des erreurs
    {
        "name": "get_error_history",
        "description": """Récupère l'historique des erreurs/bugs associés à un fichier,
un symbole, ou un module entier.

Utile pour :
- Identifier les zones à risque ("ce fichier a eu 5 bugs memory_leak")
- Comprendre les problèmes récurrents
- Contextualiser une revue de code

Exemple:
{
  "query": {"file_path": "src/lcd/write.c"},
  "errors": [
    {
      "id": 42,
      "type": "buffer_overflow",
      "severity": "critical",
      "description": "Overflow dans lcd_write_string",
      "date_reported": "2024-01-15",
      "date_fixed": "2024-01-16",
      "commit_fix": "abc123",
      "symbol_name": "lcd_write_string",
      "line": 78
    }
  ],
  "summary": {
    "total": 3,
    "by_type": {"buffer_overflow": 1, "null_pointer": 2},
    "by_severity": {"critical": 1, "medium": 2}
  }
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "file_path": {
                    "type": "string",
                    "description": "Filtrer par fichier spécifique"
                },
                "symbol_name": {
                    "type": "string",
                    "description": "Filtrer par symbole spécifique"
                },
                "module": {
                    "type": "string",
                    "description": "Filtrer par module (ex: 'lcd', 'hal')"
                },
                "error_type": {
                    "type": "string",
                    "description": "Filtrer par type d'erreur (ex: 'null_pointer', 'buffer_overflow')"
                },
                "severity": {
                    "type": "string",
                    "enum": ["critical", "high", "medium", "low"],
                    "description": "Sévérité minimum à inclure"
                },
                "days": {
                    "type": "integer",
                    "default": 180,
                    "description": "Période à considérer (en jours)"
                },
                "limit": {
                    "type": "integer",
                    "default": 20,
                    "description": "Nombre maximum de résultats"
                }
            }
        }
    },

    # 6. get_patterns - Patterns de code applicables
    {
        "name": "get_patterns",
        "description": """Récupère les patterns de code applicables à un fichier ou module.

Les patterns définissent les conventions et bonnes pratiques à suivre :
- error_handling : Comment gérer les erreurs dans ce module
- naming : Conventions de nommage
- memory : Règles d'allocation/libération mémoire
- documentation : Standards de documentation

Exemple:
{
  "query": {"file_path": "src/lcd/init.c"},
  "patterns": [
    {
      "name": "error_handling",
      "category": "reliability",
      "description": "Toute fonction qui peut échouer doit retourner un code d'erreur",
      "example": "int lcd_init(LCD_Config* cfg) { if (!cfg) return LCD_ERR_NULL; ... }",
      "severity": "high"
    },
    {
      "name": "resource_cleanup",
      "category": "memory",
      "description": "Les ressources allouées doivent être libérées en cas d'erreur",
      "example": "voir src/lcd/cleanup.c:lcd_cleanup_on_error"
    }
  ]
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "file_path": {
                    "type": "string",
                    "description": "Fichier pour lequel récupérer les patterns applicables"
                },
                "module": {
                    "type": "string",
                    "description": "Module pour lequel récupérer les patterns"
                },
                "category": {
                    "type": "string",
                    "enum": ["error_handling", "memory", "naming", "documentation", "security", "performance"],
                    "description": "Catégorie de patterns à récupérer"
                },
                "include_examples": {
                    "type": "boolean",
                    "default": True,
                    "description": "Inclure des exemples de code"
                }
            }
        }
    },

    # 7. get_architecture_decisions - ADRs applicables
    {
        "name": "get_architecture_decisions",
        "description": """Récupère les Architecture Decision Records (ADR) applicables.

Les ADRs documentent les décisions architecturales importantes :
- Pourquoi ce choix a été fait
- Quelles alternatives ont été considérées
- Quel est le contexte

Exemple:
{
  "query": {"module": "lcd"},
  "decisions": [
    {
      "id": "ADR-003",
      "title": "Utilisation d'un buffer double pour l'affichage LCD",
      "status": "accepted",
      "date": "2023-06-15",
      "context": "L'affichage direct cause du scintillement",
      "decision": "Implémenter un double buffering",
      "consequences": "Utilisation mémoire doublée, mais affichage fluide",
      "applies_to": ["src/lcd/*"]
    }
  ]
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "module": {
                    "type": "string",
                    "description": "Filtrer par module concerné"
                },
                "file_path": {
                    "type": "string",
                    "description": "Filtrer par fichier concerné"
                },
                "status": {
                    "type": "string",
                    "enum": ["accepted", "proposed", "deprecated", "superseded"],
                    "default": "accepted",
                    "description": "Statut des décisions à inclure"
                },
                "include_superseded": {
                    "type": "boolean",
                    "default": False,
                    "description": "Inclure les décisions remplacées"
                }
            }
        }
    },

    # 8. search_symbols - Recherche de symboles
    {
        "name": "search_symbols",
        "description": """Recherche des symboles par nom, type, ou pattern.

Supporte les wildcards (* pour plusieurs caractères, ? pour un seul).

Exemple:
{
  "query": "lcd_*",
  "results": [
    {"name": "lcd_init", "kind": "function", "file": "src/lcd/init.c", "line": 42},
    {"name": "lcd_write", "kind": "function", "file": "src/lcd/io.c", "line": 15},
    {"name": "LCD_Config", "kind": "struct", "file": "src/lcd/types.h", "line": 8}
  ],
  "total": 3
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "Pattern de recherche (supporte * et ? comme wildcards)"
                },
                "kind": {
                    "type": "string",
                    "enum": ["function", "struct", "class", "enum", "macro", "variable", "typedef"],
                    "description": "Type de symbole à rechercher"
                },
                "module": {
                    "type": "string",
                    "description": "Limiter la recherche à un module"
                },
                "file_path": {
                    "type": "string",
                    "description": "Limiter la recherche à un fichier"
                },
                "limit": {
                    "type": "integer",
                    "default": 50,
                    "minimum": 1,
                    "maximum": 200,
                    "description": "Nombre maximum de résultats"
                }
            },
            "required": ["query"]
        }
    },

    # 9. get_file_metrics - Métriques détaillées
    {
        "name": "get_file_metrics",
        "description": """Récupère les métriques détaillées d'un fichier.

Inclut :
- Métriques de taille (lignes total, code, commentaires, blanches)
- Métriques de complexité (cyclomatique, par fonction)
- Activité Git (commits récents, contributeurs)
- Score de risque calculé

Exemple:
{
  "file": "src/lcd/write.c",
  "metrics": {
    "lines": {"total": 250, "code": 180, "comment": 45, "blank": 25},
    "complexity": {"sum": 45, "avg": 9, "max": 18, "functions_over_10": 2},
    "git": {
      "commits_30d": 5,
      "commits_90d": 12,
      "contributors": ["alice", "bob"],
      "last_modified": "2024-01-20"
    },
    "risk_score": 0.65,
    "risk_factors": ["high_complexity", "frequent_changes", "past_bugs"]
  }
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "description": "Chemin du fichier"
                },
                "include_per_function": {
                    "type": "boolean",
                    "default": False,
                    "description": "Inclure les métriques par fonction (plus détaillé)"
                }
            },
            "required": ["path"]
        }
    },

    # 10. get_module_summary - Résumé d'un module
    {
        "name": "get_module_summary",
        "description": """Récupère un résumé complet d'un module (ensemble de fichiers).

Agrège les informations de tous les fichiers du module :
- Liste des fichiers
- Statistiques globales
- Symboles publics (API du module)
- Dépendances inter-modules
- Historique des erreurs agrégé

Exemple:
{
  "module": "lcd",
  "files": ["src/lcd/init.c", "src/lcd/write.c", "src/lcd/types.h"],
  "stats": {
    "total_files": 5,
    "total_lines": 850,
    "total_functions": 23,
    "avg_complexity": 8.5
  },
  "public_api": [
    {"name": "lcd_init", "signature": "int lcd_init(LCD_Config*)"},
    {"name": "lcd_write", "signature": "int lcd_write(const char*, size_t)"}
  ],
  "dependencies": {
    "uses": ["hal", "utils"],
    "used_by": ["ui", "main"]
  },
  "health": {
    "error_count_90d": 3,
    "test_coverage": 78,
    "documentation_score": 85
  }
}""",
        "inputSchema": {
            "type": "object",
            "properties": {
                "module": {
                    "type": "string",
                    "description": "Nom du module (correspond généralement au sous-dossier dans src/)"
                },
                "include_private": {
                    "type": "boolean",
                    "default": False,
                    "description": "Inclure les symboles privés (static) dans le résumé"
                }
            },
            "required": ["module"]
        }
    }
]


# =============================================================================
# MCP SERVER CLASS
# =============================================================================

class AgentDBServer:
    """
    Serveur MCP pour AgentDB.

    Implémente le Model Context Protocol pour exposer les fonctionnalités
    d'AgentDB aux agents Claude via transport stdio.

    Attributes:
        db_path: Chemin vers la base SQLite
        config_path: Chemin vers la configuration YAML
        db: Instance de Database (initialisée au démarrage)
    """

    def __init__(
        self,
        db_path: Optional[str] = None,
        config_path: Optional[str] = None
    ) -> None:
        """
        Initialise le serveur.

        Args:
            db_path: Chemin vers la base SQLite (défaut: env AGENTDB_PATH ou .claude/agentdb/db.sqlite)
            config_path: Chemin vers la config YAML (défaut: .claude/config/agentdb.yaml)
        """
        self.db_path = db_path or os.environ.get(
            "AGENTDB_PATH",
            ".claude/agentdb/db.sqlite"
        )
        self.config_path = config_path or os.environ.get(
            "AGENTDB_CONFIG",
            ".claude/config/agentdb.yaml"
        )

        # Initialisés au démarrage
        self.db = None
        self.config = None
        self._initialized = False

        # Mapping des outils vers leurs handlers
        self.tool_handlers: dict[str, Callable] = {}

        logger.debug(f"AgentDBServer created with db_path={self.db_path}")

    def initialize(self) -> None:
        """
        Initialise la connexion DB et les handlers.

        Appelé automatiquement par run() ou peut être appelé manuellement.
        """
        if self._initialized:
            return

        logger.info(f"Initializing AgentDB server...")
        logger.info(f"  Database: {self.db_path}")
        logger.info(f"  Config: {self.config_path}")

        # Charger la configuration
        try:
            from agentdb.config import load_config
            self.config = load_config(self.config_path)
            logger.info(f"  Project: {self.config.project.name}")
        except Exception as e:
            logger.warning(f"Could not load config: {e}, using defaults")
            self.config = None

        # Connecter à la base de données
        try:
            from agentdb.db import DatabaseManager
            self.db = DatabaseManager(self.db_path)
            self.db.connect()
            logger.info("  Database connected")
        except Exception as e:
            logger.error(f"Failed to connect to database: {e}")
            raise

        # Enregistrer les handlers
        self._register_handlers()

        self._initialized = True
        logger.info("AgentDB server initialized successfully")

    def _register_handlers(self) -> None:
        """Enregistre les handlers pour chaque outil."""
        self.tool_handlers = {
            "get_file_context": self._handle_get_file_context,
            "get_symbol_callers": self._handle_get_symbol_callers,
            "get_symbol_callees": self._handle_get_symbol_callees,
            "get_file_impact": self._handle_get_file_impact,
            "get_error_history": self._handle_get_error_history,
            "get_patterns": self._handle_get_patterns,
            "get_architecture_decisions": self._handle_get_architecture_decisions,
            "search_symbols": self._handle_search_symbols,
            "get_file_metrics": self._handle_get_file_metrics,
            "get_module_summary": self._handle_get_module_summary,
        }

    # =========================================================================
    # TOOL HANDLERS
    # =========================================================================

    async def _handle_get_file_context(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour get_file_context."""
        from . import tools
        return tools.get_file_context(
            self.db,
            path=arguments["path"],
            include_symbols=arguments.get("include_symbols", True),
            include_dependencies=arguments.get("include_dependencies", True),
            include_history=arguments.get("include_history", True),
            include_patterns=arguments.get("include_patterns", True),
        )

    async def _handle_get_symbol_callers(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour get_symbol_callers."""
        from . import tools
        return tools.get_symbol_callers(
            self.db,
            symbol_name=arguments["symbol_name"],
            file_path=arguments.get("file_path"),
            max_depth=arguments.get("max_depth", 3),
            include_indirect=arguments.get("include_indirect", True),
        )

    async def _handle_get_symbol_callees(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour get_symbol_callees."""
        from . import tools
        return tools.get_symbol_callees(
            self.db,
            symbol_name=arguments["symbol_name"],
            file_path=arguments.get("file_path"),
            max_depth=arguments.get("max_depth", 2),
        )

    async def _handle_get_file_impact(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour get_file_impact."""
        from . import tools
        return tools.get_file_impact(
            self.db,
            path=arguments["path"],
            include_transitive=arguments.get("include_transitive", True),
            max_depth=arguments.get("max_depth", 3),
        )

    async def _handle_get_error_history(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour get_error_history."""
        from . import tools
        return tools.get_error_history(
            self.db,
            file_path=arguments.get("file_path"),
            symbol_name=arguments.get("symbol_name"),
            module=arguments.get("module"),
            error_type=arguments.get("error_type"),
            severity=arguments.get("severity"),
            days=arguments.get("days", 180),
            limit=arguments.get("limit", 20),
        )

    async def _handle_get_patterns(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour get_patterns."""
        from . import tools
        return tools.get_patterns(
            self.db,
            file_path=arguments.get("file_path"),
            module=arguments.get("module"),
            category=arguments.get("category"),
            include_examples=arguments.get("include_examples", True),
        )

    async def _handle_get_architecture_decisions(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour get_architecture_decisions."""
        from . import tools
        return tools.get_architecture_decisions(
            self.db,
            module=arguments.get("module"),
            file_path=arguments.get("file_path"),
            status=arguments.get("status", "accepted"),
            include_superseded=arguments.get("include_superseded", False),
        )

    async def _handle_search_symbols(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour search_symbols."""
        from . import tools
        return tools.search_symbols(
            self.db,
            query=arguments["query"],
            kind=arguments.get("kind"),
            module=arguments.get("module"),
            file_path=arguments.get("file_path"),
            limit=arguments.get("limit", 50),
        )

    async def _handle_get_file_metrics(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour get_file_metrics."""
        from . import tools
        return tools.get_file_metrics(
            self.db,
            path=arguments["path"],
            include_per_function=arguments.get("include_per_function", False),
        )

    async def _handle_get_module_summary(self, arguments: dict[str, Any]) -> dict[str, Any]:
        """Handler pour get_module_summary."""
        from . import tools
        return tools.get_module_summary(
            self.db,
            module=arguments["module"],
            include_private=arguments.get("include_private", False),
        )

    # =========================================================================
    # MCP PROTOCOL
    # =========================================================================

    async def handle_request(self, request: dict[str, Any]) -> dict[str, Any]:
        """
        Traite une requête JSON-RPC 2.0.

        Args:
            request: Requête JSON-RPC avec id, method, params

        Returns:
            Réponse JSON-RPC
        """
        request_id = request.get("id")
        method = request.get("method", "")
        params = request.get("params", {})

        logger.debug(f"Handling request: method={method}, id={request_id}")

        try:
            # Dispatch selon la méthode
            if method == "initialize":
                result = await self._handle_initialize(params)
            elif method == "initialized":
                result = {}  # Notification, pas de réponse
            elif method == "tools/list":
                result = await self._handle_tools_list(params)
            elif method == "tools/call":
                result = await self._handle_tools_call(params)
            elif method == "shutdown":
                result = await self._handle_shutdown(params)
            else:
                return self._error_response(request_id, METHOD_NOT_FOUND, f"Unknown method: {method}")

            return self._success_response(request_id, result)

        except Exception as e:
            logger.error(f"Error handling request: {e}", exc_info=True)
            return self._error_response(request_id, INTERNAL_ERROR, str(e))

    async def _handle_initialize(self, params: dict[str, Any]) -> dict[str, Any]:
        """Handle initialize request."""
        return {
            "protocolVersion": "2024-11-05",
            "serverInfo": {
                "name": SERVER_NAME,
                "version": SERVER_VERSION,
            },
            "capabilities": {
                "tools": {
                    "listChanged": False
                }
            }
        }

    async def _handle_tools_list(self, params: dict[str, Any]) -> dict[str, Any]:
        """Handle tools/list request."""
        return {
            "tools": TOOL_DEFINITIONS
        }

    async def _handle_tools_call(self, params: dict[str, Any]) -> dict[str, Any]:
        """Handle tools/call request."""
        tool_name = params.get("name")
        arguments = params.get("arguments", {})

        if tool_name not in self.tool_handlers:
            raise ValueError(f"Unknown tool: {tool_name}")

        handler = self.tool_handlers[tool_name]
        result = await handler(arguments)

        return {
            "content": [
                {
                    "type": "text",
                    "text": json.dumps(result, indent=2, ensure_ascii=False)
                }
            ]
        }

    async def _handle_shutdown(self, params: dict[str, Any]) -> dict[str, Any]:
        """Handle shutdown request."""
        self.shutdown()
        return {}

    def _success_response(self, request_id: Any, result: Any) -> dict[str, Any]:
        """Build a success response."""
        return {
            "jsonrpc": "2.0",
            "id": request_id,
            "result": result
        }

    def _error_response(self, request_id: Any, code: int, message: str, data: Any = None) -> dict[str, Any]:
        """Build an error response."""
        error = {
            "code": code,
            "message": message
        }
        if data is not None:
            error["data"] = data
        return {
            "jsonrpc": "2.0",
            "id": request_id,
            "error": error
        }

    # =========================================================================
    # STDIO TRANSPORT
    # =========================================================================

    async def run(self) -> None:
        """
        Lance le serveur en mode stdio.

        Lit les requêtes JSON-RPC depuis stdin et écrit les réponses sur stdout.
        """
        logger.info("Starting AgentDB MCP server (stdio transport)...")

        # Initialiser si pas déjà fait
        self.initialize()

        # Configurer les streams
        reader = asyncio.StreamReader()
        protocol = asyncio.StreamReaderProtocol(reader)
        await asyncio.get_event_loop().connect_read_pipe(lambda: protocol, sys.stdin)

        writer_transport, writer_protocol = await asyncio.get_event_loop().connect_write_pipe(
            asyncio.streams.FlowControlMixin, sys.stdout
        )
        writer = asyncio.StreamWriter(writer_transport, writer_protocol, reader, asyncio.get_event_loop())

        logger.info("Server ready, waiting for requests...")

        try:
            while True:
                # Lire une ligne (requête JSON-RPC)
                line = await reader.readline()
                if not line:
                    logger.info("EOF received, shutting down")
                    break

                line = line.decode("utf-8").strip()
                if not line:
                    continue

                logger.debug(f"Received: {line[:200]}...")

                try:
                    request = json.loads(line)
                    response = await self.handle_request(request)

                    if response:  # Certaines notifications n'ont pas de réponse
                        response_str = json.dumps(response, ensure_ascii=False) + "\n"
                        writer.write(response_str.encode("utf-8"))
                        await writer.drain()
                        logger.debug(f"Sent: {response_str[:200]}...")

                except json.JSONDecodeError as e:
                    error_response = self._error_response(None, PARSE_ERROR, f"Invalid JSON: {e}")
                    writer.write((json.dumps(error_response) + "\n").encode("utf-8"))
                    await writer.drain()

        except KeyboardInterrupt:
            logger.info("Server stopped by user")
        except Exception as e:
            logger.error(f"Server error: {e}", exc_info=True)
            raise
        finally:
            self.shutdown()

    def shutdown(self) -> None:
        """Arrête proprement le serveur."""
        logger.info("Shutting down AgentDB server...")
        if self.db:
            try:
                self.db.close()
                logger.info("Database connection closed")
            except Exception as e:
                logger.error(f"Error closing database: {e}")
        self._initialized = False


# =============================================================================
# ENTRY POINT
# =============================================================================

def main() -> None:
    """Point d'entrée du serveur MCP."""
    import argparse

    parser = argparse.ArgumentParser(description="AgentDB MCP Server")
    parser.add_argument(
        "--db", "-d",
        help="Path to SQLite database",
        default=None
    )
    parser.add_argument(
        "--config", "-c",
        help="Path to YAML config file",
        default=None
    )
    parser.add_argument(
        "--log-level", "-l",
        help="Logging level",
        choices=["DEBUG", "INFO", "WARNING", "ERROR"],
        default="INFO"
    )
    args = parser.parse_args()

    # Configurer le logging
    logging.getLogger("agentdb").setLevel(args.log_level)

    # Créer et lancer le serveur
    server = AgentDBServer(db_path=args.db, config_path=args.config)
    asyncio.run(server.run())


if __name__ == "__main__":
    main()
