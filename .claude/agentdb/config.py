"""
AgentDB - Gestion de la configuration.

Ce module charge et valide la configuration d'AgentDB depuis un fichier YAML.
Il fournit des valeurs par défaut si le fichier est absent ou incomplet.

Usage:
    from agentdb.config import Config, load_config

    # Charger la config par défaut
    config = load_config()

    # Charger depuis un fichier spécifique
    config = load_config("/path/to/agentdb.yaml")

    # Accéder aux valeurs
    print(config.project.name)
    print(config.indexing.extensions)

    # Obtenir les patterns d'exclusion compilés
    patterns = config.get_exclude_patterns()
"""

from __future__ import annotations

import fnmatch
import logging
import os
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Optional, Pattern

logger = logging.getLogger("agentdb.config")

# Chemin par défaut du fichier de configuration
DEFAULT_CONFIG_PATH = ".claude/config/agentdb.yaml"


# =============================================================================
# DATACLASSES DE CONFIGURATION
# =============================================================================

@dataclass
class ProjectConfig:
    """Configuration du projet."""
    name: str = "unnamed"
    description: str = ""
    version: str = "1.0.0"
    language: str = "c"
    root: str = "."


@dataclass
class DatabaseConfig:
    """Configuration de la base de données."""
    path: str = ".claude/agentdb/db.sqlite"
    wal_mode: bool = True
    timeout: int = 30
    cache_size: int = 10000


@dataclass
class IndexingConfig:
    """Configuration de l'indexation."""
    extensions: dict[str, list[str]] = field(default_factory=dict)
    exclude: list[str] = field(default_factory=list)
    tools: dict[str, str] = field(default_factory=dict)
    ctags_path: Optional[str] = None
    max_file_size: int = 1048576  # 1 MB
    parallel_workers: int = 4
    file_timeout: int = 30

    def __post_init__(self):
        if not self.extensions:
            self.extensions = {
                "c": [".c", ".h"],
                "cpp": [".cpp", ".hpp", ".cc", ".hh", ".cxx", ".hxx"],
                "python": [".py", ".pyi"],
                "javascript": [".js", ".jsx", ".mjs"],
                "typescript": [".ts", ".tsx"],
            }
        if not self.exclude:
            self.exclude = [
                "build/**", "dist/**", "vendor/**", "node_modules/**",
                ".git/**", "__pycache__/**", "*.pyc", ".claude/agentdb/**",
            ]
        if not self.tools:
            self.tools = {
                "c": "ctags",
                "cpp": "ctags",
                "python": "ast",
                "javascript": "ctags",
            }


@dataclass
class CriticalityConfig:
    """Configuration de la criticité."""
    critical_paths: list[str] = field(default_factory=list)
    high_importance_paths: list[str] = field(default_factory=list)
    sensitive_content_patterns: list[str] = field(default_factory=list)

    def __post_init__(self):
        if not self.critical_paths:
            self.critical_paths = [
                "**/security/**", "**/auth/**", "**/crypto/**",
                "**/*password*", "**/*secret*", "**/*token*",
            ]
        if not self.high_importance_paths:
            self.high_importance_paths = [
                "**/core/**", "**/api/**", "**/main.*",
            ]
        if not self.sensitive_content_patterns:
            self.sensitive_content_patterns = [
                "password", "secret", "private_key", "api_key",
            ]


@dataclass
class ComplexityThresholds:
    """Seuils de complexité."""
    low: int = 5
    medium: int = 10
    high: int = 20
    critical: int = 30


@dataclass
class DocumentationThresholds:
    """Seuils de documentation."""
    min_public_documented: int = 80
    min_classes_documented: int = 90
    min_docstring_length: int = 10


@dataclass
class LengthThresholds:
    """Seuils de longueur."""
    ideal: int = 30
    warning: int = 50
    error: int = 100
    critical: int = 200


@dataclass
class NestingThresholds:
    """Seuils de profondeur d'imbrication."""
    ideal: int = 3
    warning: int = 5
    critical: int = 7


@dataclass
class CommentRatioThresholds:
    """Seuils de ratio commentaires/code."""
    min: float = 0.1
    ideal: float = 0.2
    max: float = 0.5


@dataclass
class MetricsConfig:
    """Configuration des métriques."""
    complexity: ComplexityThresholds = field(default_factory=ComplexityThresholds)
    documentation: DocumentationThresholds = field(default_factory=DocumentationThresholds)
    function_length: LengthThresholds = field(default_factory=LengthThresholds)
    file_length: LengthThresholds = field(default_factory=lambda: LengthThresholds(
        ideal=300, warning=500, error=1000, critical=2000
    ))
    nesting_depth: NestingThresholds = field(default_factory=NestingThresholds)
    comment_ratio: CommentRatioThresholds = field(default_factory=CommentRatioThresholds)


@dataclass
class GitActivityPeriods:
    """Périodes d'analyse Git."""
    recent: int = 30
    short_term: int = 90
    long_term: int = 365


@dataclass
class HotFileThresholds:
    """Seuils pour les hot files."""
    recent_commits: int = 5
    short_term_commits: int = 15


@dataclass
class GitConfig:
    """Configuration de l'analyse Git."""
    activity_periods: GitActivityPeriods = field(default_factory=GitActivityPeriods)
    hot_file_thresholds: HotFileThresholds = field(default_factory=HotFileThresholds)
    ignore_authors: list[str] = field(default_factory=list)
    analyze_blame: bool = True
    ignore_merge_commits: bool = True
    max_commits_per_file: int = 100

    def __post_init__(self):
        if not self.ignore_authors:
            self.ignore_authors = [
                "dependabot[bot]", "renovate[bot]", "github-actions[bot]",
            ]


@dataclass
class PatternsConfig:
    """Configuration des patterns."""
    enabled_categories: list[str] = field(default_factory=list)
    default_severity: str = "warning"
    custom: list[dict[str, Any]] = field(default_factory=list)

    def __post_init__(self):
        if not self.enabled_categories:
            self.enabled_categories = [
                "error_handling", "memory_safety", "naming_convention",
                "documentation", "security", "performance",
            ]


@dataclass
class MCPConfig:
    """Configuration du serveur MCP."""
    mode: str = "stdio"
    port: Optional[int] = None
    log_level: str = "INFO"
    request_timeout: int = 30
    cache_ttl: int = 60


@dataclass
class MaintenanceConfig:
    """Configuration de la maintenance."""
    snapshot_retention_days: int = 30
    keep_detailed_runs: int = 100
    auto_vacuum_threshold: int = 1000


@dataclass
class LoggingConfig:
    """Configuration du logging."""
    directory: str = ".claude/logs"
    level: str = "INFO"
    format: str = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
    max_size_mb: int = 10
    backup_count: int = 5


# =============================================================================
# CLASSE PRINCIPALE DE CONFIGURATION
# =============================================================================

@dataclass
class Config:
    """
    Configuration complète d'AgentDB.

    Cette classe charge et valide la configuration depuis un fichier YAML.
    Elle fournit des valeurs par défaut pour tous les champs.

    Attributes:
        project: Informations sur le projet
        database: Configuration de la base de données
        indexing: Configuration de l'indexation
        criticality: Configuration de la criticité
        metrics: Seuils des métriques
        git: Configuration de l'analyse Git
        patterns: Configuration des patterns
        mcp: Configuration du serveur MCP
        maintenance: Configuration de la maintenance
        logging: Configuration du logging

    Example:
        >>> config = Config.load(".claude/config/agentdb.yaml")
        >>> print(config.project.name)
        'flow'
        >>> print(config.indexing.extensions["c"])
        ['.c', '.h']
    """
    project: ProjectConfig = field(default_factory=ProjectConfig)
    database: DatabaseConfig = field(default_factory=DatabaseConfig)
    indexing: IndexingConfig = field(default_factory=IndexingConfig)
    criticality: CriticalityConfig = field(default_factory=CriticalityConfig)
    metrics: MetricsConfig = field(default_factory=MetricsConfig)
    git: GitConfig = field(default_factory=GitConfig)
    patterns: PatternsConfig = field(default_factory=PatternsConfig)
    mcp: MCPConfig = field(default_factory=MCPConfig)
    maintenance: MaintenanceConfig = field(default_factory=MaintenanceConfig)
    logging: LoggingConfig = field(default_factory=LoggingConfig)

    # Cache interne pour les patterns compilés
    _exclude_patterns_compiled: list[Pattern] = field(
        default_factory=list, repr=False, compare=False
    )
    _critical_patterns_compiled: list[Pattern] = field(
        default_factory=list, repr=False, compare=False
    )

    @classmethod
    def load(cls, config_path: Optional[str] = None) -> "Config":
        """
        Charge la configuration depuis un fichier YAML.

        Args:
            config_path: Chemin vers le fichier YAML (utilise le défaut si None)

        Returns:
            Instance de Config

        Raises:
            FileNotFoundError: Si le fichier n'existe pas et strict=True
            ValueError: Si le YAML est invalide
        """
        if config_path is None:
            config_path = DEFAULT_CONFIG_PATH

        path = Path(config_path)

        if not path.exists():
            logger.warning(f"Config file not found: {config_path}, using defaults")
            return cls()

        try:
            import yaml
        except ImportError:
            logger.error("PyYAML not installed. Run: pip install pyyaml")
            return cls()

        try:
            with open(path, "r", encoding="utf-8") as f:
                data = yaml.safe_load(f)
        except yaml.YAMLError as e:
            logger.error(f"Invalid YAML in {config_path}: {e}")
            raise ValueError(f"Invalid YAML: {e}") from e

        if data is None:
            logger.warning(f"Empty config file: {config_path}")
            return cls()

        # Substituer les variables d'environnement
        data = _substitute_env_vars(data)

        return cls._from_dict(data)

    @classmethod
    def _from_dict(cls, data: dict[str, Any]) -> "Config":
        """Crée une Config depuis un dictionnaire."""
        config = cls()

        # Project
        if "project" in data:
            p = data["project"]
            config.project = ProjectConfig(
                name=p.get("name", "unnamed"),
                description=p.get("description", ""),
                version=p.get("version", "1.0.0"),
                language=p.get("language", "c"),
                root=p.get("root", "."),
            )

        # Database
        if "database" in data:
            d = data["database"]
            config.database = DatabaseConfig(
                path=d.get("path", ".claude/agentdb/db.sqlite"),
                wal_mode=d.get("wal_mode", True),
                timeout=d.get("timeout", 30),
                cache_size=d.get("cache_size", 10000),
            )

        # Indexing
        if "indexing" in data:
            i = data["indexing"]
            config.indexing = IndexingConfig(
                extensions=i.get("extensions", {}),
                exclude=i.get("exclude", []),
                tools=i.get("tools", {}),
                ctags_path=i.get("ctags_path"),
                max_file_size=i.get("max_file_size", 1048576),
                parallel_workers=i.get("parallel_workers", 4),
                file_timeout=i.get("file_timeout", 30),
            )

        # Criticality
        if "criticality" in data:
            c = data["criticality"]
            config.criticality = CriticalityConfig(
                critical_paths=c.get("critical_paths", []),
                high_importance_paths=c.get("high_importance_paths", []),
                sensitive_content_patterns=c.get("sensitive_content_patterns", []),
            )

        # Metrics
        if "metrics" in data:
            m = data["metrics"]
            config.metrics = _parse_metrics(m)

        # Git
        if "git" in data:
            g = data["git"]
            activity = g.get("activity_periods", {})
            hot = g.get("hot_file_thresholds", {})
            config.git = GitConfig(
                activity_periods=GitActivityPeriods(
                    recent=activity.get("recent", 30),
                    short_term=activity.get("short_term", 90),
                    long_term=activity.get("long_term", 365),
                ),
                hot_file_thresholds=HotFileThresholds(
                    recent_commits=hot.get("recent_commits", 5),
                    short_term_commits=hot.get("short_term_commits", 15),
                ),
                ignore_authors=g.get("ignore_authors", []),
                analyze_blame=g.get("analyze_blame", True),
                ignore_merge_commits=g.get("ignore_merge_commits", True),
                max_commits_per_file=g.get("max_commits_per_file", 100),
            )

        # Patterns
        if "patterns" in data:
            p = data["patterns"]
            config.patterns = PatternsConfig(
                enabled_categories=p.get("enabled_categories", []),
                default_severity=p.get("default_severity", "warning"),
                custom=p.get("custom", []),
            )

        # MCP
        if "mcp" in data:
            m = data["mcp"]
            config.mcp = MCPConfig(
                mode=m.get("mode", "stdio"),
                port=m.get("port"),
                log_level=m.get("log_level", "INFO"),
                request_timeout=m.get("request_timeout", 30),
                cache_ttl=m.get("cache_ttl", 60),
            )

        # Maintenance
        if "maintenance" in data:
            m = data["maintenance"]
            config.maintenance = MaintenanceConfig(
                snapshot_retention_days=m.get("snapshot_retention_days", 30),
                keep_detailed_runs=m.get("keep_detailed_runs", 100),
                auto_vacuum_threshold=m.get("auto_vacuum_threshold", 1000),
            )

        # Logging
        if "logging" in data:
            l = data["logging"]
            config.logging = LoggingConfig(
                directory=l.get("directory", ".claude/logs"),
                level=l.get("level", "INFO"),
                format=l.get("format", "%(asctime)s - %(name)s - %(levelname)s - %(message)s"),
                max_size_mb=l.get("max_size_mb", 10),
                backup_count=l.get("backup_count", 5),
            )

        # Valider
        config.validate()

        return config

    def validate(self) -> list[str]:
        """
        Valide la configuration et retourne les erreurs.

        Returns:
            Liste des messages d'erreur (vide si tout est OK)

        Raises:
            ValueError: Si des champs obligatoires sont manquants
        """
        errors = []

        # Champs obligatoires
        if not self.project.name:
            errors.append("project.name is required")

        if not self.project.language:
            errors.append("project.language is required")

        if self.project.language not in ["c", "cpp", "python", "javascript", "typescript", "rust", "go", "java"]:
            errors.append(f"project.language '{self.project.language}' is not supported")

        # Vérifier que les extensions contiennent le langage principal
        if self.project.language not in self.indexing.extensions:
            errors.append(
                f"project.language '{self.project.language}' not found in indexing.extensions"
            )

        # Vérifier les seuils de métriques
        cx = self.metrics.complexity
        if not (cx.low < cx.medium < cx.high < cx.critical):
            errors.append("complexity thresholds must be in ascending order")

        # Vérifier les périodes Git
        ap = self.git.activity_periods
        if not (ap.recent < ap.short_term < ap.long_term):
            errors.append("git.activity_periods must be in ascending order")

        if errors:
            for e in errors:
                logger.error(f"Config validation error: {e}")
            raise ValueError(f"Configuration errors: {errors}")

        return errors

    def get_exclude_patterns(self) -> list[re.Pattern]:
        """
        Retourne les patterns d'exclusion compilés en regex.

        Les patterns glob sont convertis en expressions régulières.

        Returns:
            Liste de patterns regex compilés

        Example:
            >>> config = Config.load()
            >>> patterns = config.get_exclude_patterns()
            >>> any(p.match("build/output.o") for p in patterns)
            True
        """
        if self._exclude_patterns_compiled:
            return self._exclude_patterns_compiled

        self._exclude_patterns_compiled = _compile_glob_patterns(
            self.indexing.exclude
        )
        return self._exclude_patterns_compiled

    def get_critical_patterns(self) -> list[re.Pattern]:
        """
        Retourne les patterns de chemins critiques compilés.

        Returns:
            Liste de patterns regex compilés
        """
        if self._critical_patterns_compiled:
            return self._critical_patterns_compiled

        self._critical_patterns_compiled = _compile_glob_patterns(
            self.criticality.critical_paths
        )
        return self._critical_patterns_compiled

    def is_excluded(self, file_path: str) -> bool:
        """
        Vérifie si un fichier est exclu de l'indexation.

        Args:
            file_path: Chemin du fichier (relatif)

        Returns:
            True si le fichier doit être exclu
        """
        patterns = self.get_exclude_patterns()
        return any(p.match(file_path) for p in patterns)

    def is_critical(self, file_path: str) -> bool:
        """
        Vérifie si un fichier est dans un chemin critique.

        Args:
            file_path: Chemin du fichier (relatif)

        Returns:
            True si le fichier est critique
        """
        patterns = self.get_critical_patterns()
        return any(p.match(file_path) for p in patterns)

    def get_tool_for_language(self, language: str) -> str:
        """
        Retourne l'outil d'indexation pour un langage.

        Args:
            language: Nom du langage

        Returns:
            Nom de l'outil (ctags, ast, tree-sitter, etc.)
        """
        return self.indexing.tools.get(language, "ctags")

    def get_extensions_for_language(self, language: str) -> list[str]:
        """
        Retourne les extensions de fichiers pour un langage.

        Args:
            language: Nom du langage

        Returns:
            Liste des extensions (avec le point)
        """
        return self.indexing.extensions.get(language, [])

    def to_dict(self) -> dict[str, Any]:
        """Convertit la configuration en dictionnaire."""
        return {
            "project": {
                "name": self.project.name,
                "description": self.project.description,
                "version": self.project.version,
                "language": self.project.language,
                "root": self.project.root,
            },
            "database": {
                "path": self.database.path,
                "wal_mode": self.database.wal_mode,
                "timeout": self.database.timeout,
                "cache_size": self.database.cache_size,
            },
            "indexing": {
                "extensions": self.indexing.extensions,
                "exclude": self.indexing.exclude,
                "tools": self.indexing.tools,
                "ctags_path": self.indexing.ctags_path,
                "max_file_size": self.indexing.max_file_size,
                "parallel_workers": self.indexing.parallel_workers,
                "file_timeout": self.indexing.file_timeout,
            },
            "criticality": {
                "critical_paths": self.criticality.critical_paths,
                "high_importance_paths": self.criticality.high_importance_paths,
                "sensitive_content_patterns": self.criticality.sensitive_content_patterns,
            },
            "git": {
                "activity_periods": {
                    "recent": self.git.activity_periods.recent,
                    "short_term": self.git.activity_periods.short_term,
                    "long_term": self.git.activity_periods.long_term,
                },
                "hot_file_thresholds": {
                    "recent_commits": self.git.hot_file_thresholds.recent_commits,
                    "short_term_commits": self.git.hot_file_thresholds.short_term_commits,
                },
            },
        }


# =============================================================================
# FONCTIONS UTILITAIRES
# =============================================================================

def _substitute_env_vars(data: Any) -> Any:
    """
    Substitue les variables d'environnement dans les valeurs.

    Supporte le format ${VAR_NAME} et ${VAR_NAME:-default}.

    Args:
        data: Données à traiter (dict, list, ou valeur)

    Returns:
        Données avec les variables substituées
    """
    if isinstance(data, dict):
        return {k: _substitute_env_vars(v) for k, v in data.items()}
    elif isinstance(data, list):
        return [_substitute_env_vars(item) for item in data]
    elif isinstance(data, str):
        # Pattern: ${VAR} ou ${VAR:-default}
        pattern = re.compile(r'\$\{([A-Z_][A-Z0-9_]*)(?::-([^}]*))?\}')

        def replace(match):
            var_name = match.group(1)
            default = match.group(2) or ""
            return os.environ.get(var_name, default)

        return pattern.sub(replace, data)
    else:
        return data


def _compile_glob_patterns(patterns: list[str]) -> list[re.Pattern]:
    """
    Compile les patterns glob en expressions régulières.

    Args:
        patterns: Liste de patterns glob

    Returns:
        Liste de patterns regex compilés
    """
    compiled = []
    for pattern in patterns:
        # Convertir glob en regex
        regex = fnmatch.translate(pattern)
        try:
            compiled.append(re.compile(regex))
        except re.error as e:
            logger.warning(f"Invalid pattern '{pattern}': {e}")
    return compiled


def _parse_metrics(data: dict[str, Any]) -> MetricsConfig:
    """Parse la section metrics du YAML."""
    config = MetricsConfig()

    if "complexity" in data:
        c = data["complexity"]
        config.complexity = ComplexityThresholds(
            low=c.get("low", 5),
            medium=c.get("medium", 10),
            high=c.get("high", 20),
            critical=c.get("critical", 30),
        )

    if "documentation" in data:
        d = data["documentation"]
        config.documentation = DocumentationThresholds(
            min_public_documented=d.get("min_public_documented", 80),
            min_classes_documented=d.get("min_classes_documented", 90),
            min_docstring_length=d.get("min_docstring_length", 10),
        )

    if "function_length" in data:
        f = data["function_length"]
        config.function_length = LengthThresholds(
            ideal=f.get("ideal", 30),
            warning=f.get("warning", 50),
            error=f.get("error", 100),
            critical=f.get("critical", 200),
        )

    if "file_length" in data:
        f = data["file_length"]
        config.file_length = LengthThresholds(
            ideal=f.get("ideal", 300),
            warning=f.get("warning", 500),
            error=f.get("error", 1000),
            critical=f.get("critical", 2000),
        )

    if "nesting_depth" in data:
        n = data["nesting_depth"]
        config.nesting_depth = NestingThresholds(
            ideal=n.get("ideal", 3),
            warning=n.get("warning", 5),
            critical=n.get("critical", 7),
        )

    if "comment_ratio" in data:
        c = data["comment_ratio"]
        config.comment_ratio = CommentRatioThresholds(
            min=c.get("min", 0.1),
            ideal=c.get("ideal", 0.2),
            max=c.get("max", 0.5),
        )

    return config


def load_config(config_path: Optional[str] = None) -> Config:
    """
    Fonction utilitaire pour charger la configuration.

    Args:
        config_path: Chemin vers le fichier YAML (optionnel)

    Returns:
        Instance de Config

    Example:
        >>> config = load_config()
        >>> print(config.project.name)
    """
    return Config.load(config_path)


# =============================================================================
# EXPORTS
# =============================================================================

__all__ = [
    # Classes principales
    "Config",
    "load_config",
    # Sous-configurations
    "ProjectConfig",
    "DatabaseConfig",
    "IndexingConfig",
    "CriticalityConfig",
    "MetricsConfig",
    "GitConfig",
    "PatternsConfig",
    "MCPConfig",
    "MaintenanceConfig",
    "LoggingConfig",
    # Seuils
    "ComplexityThresholds",
    "DocumentationThresholds",
    "LengthThresholds",
    "NestingThresholds",
    "CommentRatioThresholds",
    "GitActivityPeriods",
    "HotFileThresholds",
    # Constantes
    "DEFAULT_CONFIG_PATH",
]
