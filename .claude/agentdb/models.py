"""
AgentDB - Modèles de données (dataclasses).

Ce module définit les structures de données pour :
- Pilier 1 : Graphe (File, Symbol, Relation, FileRelation)
- Pilier 2 : Mémoire (ErrorHistory, PipelineRun, SnapshotSymbol)
- Pilier 3 : Connaissance (Pattern, ArchitectureDecision, CriticalPath)

Chaque modèle correspond à une table SQLite et peut être
sérialisé/désérialisé depuis/vers la base via from_row() et to_dict().

Usage:
    # Depuis une ligne SQLite
    file = File.from_row(row_dict)

    # Vers un dict pour insertion
    data = file.to_dict()
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field, asdict
from datetime import datetime
from enum import Enum
from typing import Any, Optional


# =============================================================================
# ENUMS
# =============================================================================

class SymbolKind(str, Enum):
    """Types de symboles supportés."""
    FUNCTION = "function"
    STRUCT = "struct"
    CLASS = "class"
    ENUM = "enum"
    TYPEDEF = "typedef"
    MACRO = "macro"
    VARIABLE = "variable"
    CONSTANT = "constant"
    INTERFACE = "interface"
    MODULE = "module"
    METHOD = "method"
    PROPERTY = "property"
    FIELD = "field"
    PARAMETER = "parameter"
    UNION = "union"
    NAMESPACE = "namespace"

    @classmethod
    def from_str(cls, value: str) -> "SymbolKind":
        """Convertit une chaîne en SymbolKind."""
        try:
            return cls(value.lower())
        except ValueError:
            return cls.VARIABLE  # Fallback


class RelationType(str, Enum):
    """Types de relations entre symboles."""
    CALLS = "calls"
    INCLUDES = "includes"
    IMPORTS = "imports"
    USES_TYPE = "uses_type"
    RETURNS_TYPE = "returns_type"
    HAS_PARAM_TYPE = "has_param_type"
    INHERITS = "inherits"
    IMPLEMENTS = "implements"
    USES_VARIABLE = "uses_variable"
    MODIFIES = "modifies"
    READS = "reads"
    INSTANTIATES = "instantiates"
    USES_MACRO = "uses_macro"
    CONTAINS = "contains"
    REFERENCES = "references"
    DEFINES = "defines"
    DECLARES = "declares"

    @classmethod
    def from_str(cls, value: str) -> "RelationType":
        """Convertit une chaîne en RelationType."""
        try:
            return cls(value.lower())
        except ValueError:
            return cls.REFERENCES  # Fallback


class Severity(str, Enum):
    """Niveaux de sévérité."""
    CRITICAL = "critical"
    HIGH = "high"
    MEDIUM = "medium"
    LOW = "low"
    INFO = "info"

    @classmethod
    def from_str(cls, value: str) -> "Severity":
        """Convertit une chaîne en Severity."""
        try:
            return cls(value.lower())
        except ValueError:
            return cls.MEDIUM  # Fallback

    def __lt__(self, other: "Severity") -> bool:
        """Permet de comparer les sévérités."""
        order = {self.CRITICAL: 4, self.HIGH: 3, self.MEDIUM: 2, self.LOW: 1, self.INFO: 0}
        return order[self] < order[other]


class ErrorType(str, Enum):
    """Types d'erreurs/vulnérabilités."""
    # Memory safety
    BUFFER_OVERFLOW = "buffer_overflow"
    NULL_POINTER = "null_pointer"
    MEMORY_LEAK = "memory_leak"
    USE_AFTER_FREE = "use_after_free"
    DOUBLE_FREE = "double_free"
    UNINITIALIZED_MEMORY = "uninitialized_memory"
    OUT_OF_BOUNDS = "out_of_bounds"

    # Concurrency
    RACE_CONDITION = "race_condition"
    DEADLOCK = "deadlock"

    # Security
    SQL_INJECTION = "sql_injection"
    XSS = "xss"
    COMMAND_INJECTION = "command_injection"
    PATH_TRAVERSAL = "path_traversal"
    AUTH_BYPASS = "auth_bypass"
    INSECURE_CRYPTO = "insecure_crypto"
    SENSITIVE_DATA_EXPOSURE = "sensitive_data_exposure"

    # Logic
    LOGIC_ERROR = "logic_error"
    INTEGER_OVERFLOW = "integer_overflow"
    DIVISION_BY_ZERO = "division_by_zero"
    INFINITE_LOOP = "infinite_loop"

    # Quality
    PERFORMANCE = "performance"
    RESOURCE_LEAK = "resource_leak"
    CRASH = "crash"
    DATA_CORRUPTION = "data_corruption"
    REGRESSION = "regression"

    # Other
    OTHER = "other"

    @classmethod
    def from_str(cls, value: str) -> "ErrorType":
        """Convertit une chaîne en ErrorType."""
        try:
            return cls(value.lower())
        except ValueError:
            return cls.OTHER


class FileType(str, Enum):
    """Types de fichiers."""
    SOURCE = "source"
    HEADER = "header"
    TEST = "test"
    CONFIG = "config"
    DOC = "doc"
    BUILD = "build"
    DATA = "data"

    @classmethod
    def from_str(cls, value: str) -> "FileType":
        """Convertit une chaîne en FileType."""
        try:
            return cls(value.lower())
        except ValueError:
            return cls.SOURCE


class Visibility(str, Enum):
    """Visibilité des symboles."""
    PUBLIC = "public"
    PRIVATE = "private"
    PROTECTED = "protected"
    INTERNAL = "internal"
    STATIC = "static"
    EXTERN = "extern"

    @classmethod
    def from_str(cls, value: str) -> "Visibility":
        """Convertit une chaîne en Visibility."""
        try:
            return cls(value.lower())
        except ValueError:
            return cls.PUBLIC


class ADRStatus(str, Enum):
    """Statut d'une décision architecturale."""
    PROPOSED = "proposed"
    ACCEPTED = "accepted"
    DEPRECATED = "deprecated"
    SUPERSEDED = "superseded"
    REJECTED = "rejected"

    @classmethod
    def from_str(cls, value: str) -> "ADRStatus":
        """Convertit une chaîne en ADRStatus."""
        try:
            return cls(value.lower())
        except ValueError:
            return cls.PROPOSED


class PipelineStatus(str, Enum):
    """Statut d'un run du pipeline."""
    PENDING = "pending"
    RUNNING = "running"
    SUCCESS = "success"
    WARNING = "warning"
    FAILED = "failed"
    CANCELLED = "cancelled"

    @classmethod
    def from_str(cls, value: str) -> "PipelineStatus":
        """Convertit une chaîne en PipelineStatus."""
        try:
            return cls(value.lower())
        except ValueError:
            return cls.PENDING


# =============================================================================
# HELPER FUNCTIONS
# =============================================================================

def _parse_json(value: Optional[str]) -> Optional[Any]:
    """Parse une chaîne JSON ou retourne None."""
    if value is None:
        return None
    try:
        return json.loads(value)
    except (json.JSONDecodeError, TypeError):
        return None


def _to_json(value: Optional[Any]) -> Optional[str]:
    """Convertit en JSON ou retourne None."""
    if value is None:
        return None
    try:
        return json.dumps(value, ensure_ascii=False)
    except (TypeError, ValueError):
        return None


# =============================================================================
# PILIER 1 : GRAPHE DE DÉPENDANCES
# =============================================================================

@dataclass(slots=True)
class File:
    """
    Représente un fichier du projet.
    Correspond à la table `files`.
    """
    # Identification
    id: Optional[int] = None
    path: str = ""
    filename: str = ""
    extension: Optional[str] = None

    # Classification
    module: Optional[str] = None
    layer: Optional[str] = None
    file_type: str = "source"
    language: Optional[str] = None

    # Criticité
    is_critical: bool = False
    criticality_reason: Optional[str] = None
    security_sensitive: bool = False

    # Métriques de code
    lines_total: int = 0
    lines_code: int = 0
    lines_comment: int = 0
    lines_blank: int = 0
    complexity_sum: int = 0
    complexity_avg: float = 0.0
    complexity_max: int = 0

    # Métriques d'activité
    commits_30d: int = 0
    commits_90d: int = 0
    commits_365d: int = 0
    contributors_json: Optional[str] = None
    last_modified: Optional[str] = None
    created_at: Optional[str] = None

    # Métriques de qualité
    has_tests: bool = False
    test_file_path: Optional[str] = None
    documentation_score: int = 0
    technical_debt_score: int = 0

    # Métadonnées
    content_hash: Optional[str] = None
    indexed_at: Optional[str] = None
    index_version: int = 1

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "File":
        """Crée une instance depuis une ligne SQLite."""
        return cls(
            id=row.get("id"),
            path=row.get("path", ""),
            filename=row.get("filename", ""),
            extension=row.get("extension"),
            module=row.get("module"),
            layer=row.get("layer"),
            file_type=row.get("file_type", "source"),
            language=row.get("language"),
            is_critical=bool(row.get("is_critical", 0)),
            criticality_reason=row.get("criticality_reason"),
            security_sensitive=bool(row.get("security_sensitive", 0)),
            lines_total=row.get("lines_total", 0) or 0,
            lines_code=row.get("lines_code", 0) or 0,
            lines_comment=row.get("lines_comment", 0) or 0,
            lines_blank=row.get("lines_blank", 0) or 0,
            complexity_sum=row.get("complexity_sum", 0) or 0,
            complexity_avg=row.get("complexity_avg", 0.0) or 0.0,
            complexity_max=row.get("complexity_max", 0) or 0,
            commits_30d=row.get("commits_30d", 0) or 0,
            commits_90d=row.get("commits_90d", 0) or 0,
            commits_365d=row.get("commits_365d", 0) or 0,
            contributors_json=row.get("contributors_json"),
            last_modified=row.get("last_modified"),
            created_at=row.get("created_at"),
            has_tests=bool(row.get("has_tests", 0)),
            test_file_path=row.get("test_file_path"),
            documentation_score=row.get("documentation_score", 0) or 0,
            technical_debt_score=row.get("technical_debt_score", 0) or 0,
            content_hash=row.get("content_hash"),
            indexed_at=row.get("indexed_at"),
            index_version=row.get("index_version", 1) or 1,
        )

    def to_dict(self, exclude_none: bool = False, exclude_id: bool = False) -> dict[str, Any]:
        """Convertit en dictionnaire pour insertion SQL."""
        d = {
            "id": self.id,
            "path": self.path,
            "filename": self.filename,
            "extension": self.extension,
            "module": self.module,
            "layer": self.layer,
            "file_type": self.file_type,
            "language": self.language,
            "is_critical": int(self.is_critical),
            "criticality_reason": self.criticality_reason,
            "security_sensitive": int(self.security_sensitive),
            "lines_total": self.lines_total,
            "lines_code": self.lines_code,
            "lines_comment": self.lines_comment,
            "lines_blank": self.lines_blank,
            "complexity_sum": self.complexity_sum,
            "complexity_avg": self.complexity_avg,
            "complexity_max": self.complexity_max,
            "commits_30d": self.commits_30d,
            "commits_90d": self.commits_90d,
            "commits_365d": self.commits_365d,
            "contributors_json": self.contributors_json,
            "last_modified": self.last_modified,
            "created_at": self.created_at,
            "has_tests": int(self.has_tests),
            "test_file_path": self.test_file_path,
            "documentation_score": self.documentation_score,
            "technical_debt_score": self.technical_debt_score,
            "content_hash": self.content_hash,
            "indexed_at": self.indexed_at,
            "index_version": self.index_version,
        }
        if exclude_id:
            d.pop("id", None)
        if exclude_none:
            d = {k: v for k, v in d.items() if v is not None}
        return d

    @property
    def contributors(self) -> list[dict[str, Any]]:
        """Parse contributors_json."""
        return _parse_json(self.contributors_json) or []

    @contributors.setter
    def contributors(self, value: list[dict[str, Any]]) -> None:
        """Sérialise contributors en JSON."""
        self.contributors_json = _to_json(value)


@dataclass(slots=True)
class Symbol:
    """
    Représente un symbole du code (fonction, type, variable, etc.).
    Correspond à la table `symbols`.
    """
    # Identification
    id: Optional[int] = None
    file_id: int = 0
    name: str = ""
    qualified_name: Optional[str] = None

    # Classification
    kind: str = "function"

    # Localisation
    line_start: Optional[int] = None
    line_end: Optional[int] = None
    column_start: Optional[int] = None
    column_end: Optional[int] = None

    # Signature (fonctions)
    signature: Optional[str] = None
    return_type: Optional[str] = None
    parameters_json: Optional[str] = None
    is_variadic: bool = False

    # Structure (struct/class/enum)
    fields_json: Optional[str] = None
    base_classes_json: Optional[str] = None
    size_bytes: Optional[int] = None

    # Visibilité
    visibility: str = "public"
    is_exported: bool = False
    is_static: bool = False
    is_inline: bool = False

    # Métriques
    complexity: int = 0
    lines_of_code: int = 0
    cognitive_complexity: int = 0
    nesting_depth: int = 0

    # Documentation
    doc_comment: Optional[str] = None
    has_doc: bool = False
    doc_quality: int = 0

    # Métadonnées
    attributes_json: Optional[str] = None
    hash: Optional[str] = None
    indexed_at: Optional[str] = None

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "Symbol":
        """Crée une instance depuis une ligne SQLite."""
        return cls(
            id=row.get("id"),
            file_id=row.get("file_id", 0) or 0,
            name=row.get("name", ""),
            qualified_name=row.get("qualified_name"),
            kind=row.get("kind", "function"),
            line_start=row.get("line_start"),
            line_end=row.get("line_end"),
            column_start=row.get("column_start"),
            column_end=row.get("column_end"),
            signature=row.get("signature"),
            return_type=row.get("return_type"),
            parameters_json=row.get("parameters_json"),
            is_variadic=bool(row.get("is_variadic", 0)),
            fields_json=row.get("fields_json"),
            base_classes_json=row.get("base_classes_json"),
            size_bytes=row.get("size_bytes"),
            visibility=row.get("visibility", "public"),
            is_exported=bool(row.get("is_exported", 0)),
            is_static=bool(row.get("is_static", 0)),
            is_inline=bool(row.get("is_inline", 0)),
            complexity=row.get("complexity", 0) or 0,
            lines_of_code=row.get("lines_of_code", 0) or 0,
            cognitive_complexity=row.get("cognitive_complexity", 0) or 0,
            nesting_depth=row.get("nesting_depth", 0) or 0,
            doc_comment=row.get("doc_comment"),
            has_doc=bool(row.get("has_doc", 0)),
            doc_quality=row.get("doc_quality", 0) or 0,
            attributes_json=row.get("attributes_json"),
            hash=row.get("hash"),
            indexed_at=row.get("indexed_at"),
        )

    def to_dict(self, exclude_none: bool = False, exclude_id: bool = False) -> dict[str, Any]:
        """Convertit en dictionnaire pour insertion SQL."""
        d = {
            "id": self.id,
            "file_id": self.file_id,
            "name": self.name,
            "qualified_name": self.qualified_name,
            "kind": self.kind,
            "line_start": self.line_start,
            "line_end": self.line_end,
            "column_start": self.column_start,
            "column_end": self.column_end,
            "signature": self.signature,
            "return_type": self.return_type,
            "parameters_json": self.parameters_json,
            "is_variadic": int(self.is_variadic),
            "fields_json": self.fields_json,
            "base_classes_json": self.base_classes_json,
            "size_bytes": self.size_bytes,
            "visibility": self.visibility,
            "is_exported": int(self.is_exported),
            "is_static": int(self.is_static),
            "is_inline": int(self.is_inline),
            "complexity": self.complexity,
            "lines_of_code": self.lines_of_code,
            "cognitive_complexity": self.cognitive_complexity,
            "nesting_depth": self.nesting_depth,
            "doc_comment": self.doc_comment,
            "has_doc": int(self.has_doc),
            "doc_quality": self.doc_quality,
            "attributes_json": self.attributes_json,
            "hash": self.hash,
            "indexed_at": self.indexed_at,
        }
        if exclude_id:
            d.pop("id", None)
        if exclude_none:
            d = {k: v for k, v in d.items() if v is not None}
        return d

    @property
    def parameters(self) -> list[dict[str, Any]]:
        """Parse parameters_json."""
        return _parse_json(self.parameters_json) or []

    @parameters.setter
    def parameters(self, value: list[dict[str, Any]]) -> None:
        """Sérialise parameters en JSON."""
        self.parameters_json = _to_json(value)

    @property
    def fields(self) -> list[dict[str, Any]]:
        """Parse fields_json."""
        return _parse_json(self.fields_json) or []

    @fields.setter
    def fields(self, value: list[dict[str, Any]]) -> None:
        """Sérialise fields en JSON."""
        self.fields_json = _to_json(value)

    @property
    def base_classes(self) -> list[str]:
        """Parse base_classes_json."""
        return _parse_json(self.base_classes_json) or []

    @base_classes.setter
    def base_classes(self, value: list[str]) -> None:
        """Sérialise base_classes en JSON."""
        self.base_classes_json = _to_json(value)

    @property
    def attributes(self) -> dict[str, Any]:
        """Parse attributes_json."""
        return _parse_json(self.attributes_json) or {}

    @attributes.setter
    def attributes(self, value: dict[str, Any]) -> None:
        """Sérialise attributes en JSON."""
        self.attributes_json = _to_json(value)

    @property
    def kind_enum(self) -> SymbolKind:
        """Retourne le kind comme enum."""
        return SymbolKind.from_str(self.kind)


@dataclass(slots=True)
class Relation:
    """
    Représente une relation entre deux symboles.
    Correspond à la table `relations`.
    """
    id: Optional[int] = None
    source_id: int = 0
    target_id: int = 0

    # Type de relation
    relation_type: str = "calls"

    # Localisation
    location_file_id: Optional[int] = None
    location_line: Optional[int] = None
    location_column: Optional[int] = None

    # Métadonnées
    count: int = 1
    is_direct: bool = True
    is_conditional: bool = False
    context: Optional[str] = None

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "Relation":
        """Crée une instance depuis une ligne SQLite."""
        return cls(
            id=row.get("id"),
            source_id=row.get("source_id", 0) or 0,
            target_id=row.get("target_id", 0) or 0,
            relation_type=row.get("relation_type", "calls"),
            location_file_id=row.get("location_file_id"),
            location_line=row.get("location_line"),
            location_column=row.get("location_column"),
            count=row.get("count", 1) or 1,
            is_direct=bool(row.get("is_direct", 1)),
            is_conditional=bool(row.get("is_conditional", 0)),
            context=row.get("context"),
        )

    def to_dict(self, exclude_none: bool = False, exclude_id: bool = False) -> dict[str, Any]:
        """Convertit en dictionnaire pour insertion SQL."""
        d = {
            "id": self.id,
            "source_id": self.source_id,
            "target_id": self.target_id,
            "relation_type": self.relation_type,
            "location_file_id": self.location_file_id,
            "location_line": self.location_line,
            "location_column": self.location_column,
            "count": self.count,
            "is_direct": int(self.is_direct),
            "is_conditional": int(self.is_conditional),
            "context": self.context,
        }
        if exclude_id:
            d.pop("id", None)
        if exclude_none:
            d = {k: v for k, v in d.items() if v is not None}
        return d

    @property
    def relation_type_enum(self) -> RelationType:
        """Retourne le relation_type comme enum."""
        return RelationType.from_str(self.relation_type)


@dataclass(slots=True)
class FileRelation:
    """
    Représente une relation entre deux fichiers.
    Correspond à la table `file_relations`.
    """
    id: Optional[int] = None
    source_file_id: int = 0
    target_file_id: int = 0
    relation_type: str = "includes"
    is_direct: bool = True
    line_number: Optional[int] = None

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "FileRelation":
        """Crée une instance depuis une ligne SQLite."""
        return cls(
            id=row.get("id"),
            source_file_id=row.get("source_file_id", 0) or 0,
            target_file_id=row.get("target_file_id", 0) or 0,
            relation_type=row.get("relation_type", "includes"),
            is_direct=bool(row.get("is_direct", 1)),
            line_number=row.get("line_number"),
        )

    def to_dict(self, exclude_none: bool = False, exclude_id: bool = False) -> dict[str, Any]:
        """Convertit en dictionnaire pour insertion SQL."""
        d = {
            "id": self.id,
            "source_file_id": self.source_file_id,
            "target_file_id": self.target_file_id,
            "relation_type": self.relation_type,
            "is_direct": int(self.is_direct),
            "line_number": self.line_number,
        }
        if exclude_id:
            d.pop("id", None)
        if exclude_none:
            d = {k: v for k, v in d.items() if v is not None}
        return d


# =============================================================================
# PILIER 2 : MÉMOIRE HISTORIQUE
# =============================================================================

@dataclass(slots=True)
class ErrorHistory:
    """
    Représente une erreur/bug historique.
    Correspond à la table `error_history`.
    """
    # Identification
    id: Optional[int] = None
    file_id: Optional[int] = None
    file_path: str = ""
    symbol_name: Optional[str] = None
    symbol_id: Optional[int] = None

    # Classification
    error_type: str = "other"
    severity: str = "medium"
    cwe_id: Optional[str] = None

    # Description
    title: str = ""
    description: Optional[str] = None
    root_cause: Optional[str] = None
    symptoms: Optional[str] = None

    # Résolution
    resolution: Optional[str] = None
    prevention: Optional[str] = None
    fix_commit: Optional[str] = None
    fix_diff: Optional[str] = None

    # Contexte
    discovered_at: str = ""
    resolved_at: Optional[str] = None
    discovered_by: Optional[str] = None
    reported_in: Optional[str] = None
    jira_ticket: Optional[str] = None
    environment: Optional[str] = None

    # Commits
    introducing_commit: Optional[str] = None
    related_commits_json: Optional[str] = None

    # Métadonnées
    is_regression: bool = False
    original_error_id: Optional[int] = None
    tags_json: Optional[str] = None
    extra_data_json: Optional[str] = None

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "ErrorHistory":
        """Crée une instance depuis une ligne SQLite."""
        return cls(
            id=row.get("id"),
            file_id=row.get("file_id"),
            file_path=row.get("file_path", ""),
            symbol_name=row.get("symbol_name"),
            symbol_id=row.get("symbol_id"),
            error_type=row.get("error_type", "other"),
            severity=row.get("severity", "medium"),
            cwe_id=row.get("cwe_id"),
            title=row.get("title", ""),
            description=row.get("description"),
            root_cause=row.get("root_cause"),
            symptoms=row.get("symptoms"),
            resolution=row.get("resolution"),
            prevention=row.get("prevention"),
            fix_commit=row.get("fix_commit"),
            fix_diff=row.get("fix_diff"),
            discovered_at=row.get("discovered_at", ""),
            resolved_at=row.get("resolved_at"),
            discovered_by=row.get("discovered_by"),
            reported_in=row.get("reported_in"),
            jira_ticket=row.get("jira_ticket"),
            environment=row.get("environment"),
            introducing_commit=row.get("introducing_commit"),
            related_commits_json=row.get("related_commits_json"),
            is_regression=bool(row.get("is_regression", 0)),
            original_error_id=row.get("original_error_id"),
            tags_json=row.get("tags_json"),
            extra_data_json=row.get("extra_data_json"),
        )

    def to_dict(self, exclude_none: bool = False, exclude_id: bool = False) -> dict[str, Any]:
        """Convertit en dictionnaire pour insertion SQL."""
        d = {
            "id": self.id,
            "file_id": self.file_id,
            "file_path": self.file_path,
            "symbol_name": self.symbol_name,
            "symbol_id": self.symbol_id,
            "error_type": self.error_type,
            "severity": self.severity,
            "cwe_id": self.cwe_id,
            "title": self.title,
            "description": self.description,
            "root_cause": self.root_cause,
            "symptoms": self.symptoms,
            "resolution": self.resolution,
            "prevention": self.prevention,
            "fix_commit": self.fix_commit,
            "fix_diff": self.fix_diff,
            "discovered_at": self.discovered_at,
            "resolved_at": self.resolved_at,
            "discovered_by": self.discovered_by,
            "reported_in": self.reported_in,
            "jira_ticket": self.jira_ticket,
            "environment": self.environment,
            "introducing_commit": self.introducing_commit,
            "related_commits_json": self.related_commits_json,
            "is_regression": int(self.is_regression),
            "original_error_id": self.original_error_id,
            "tags_json": self.tags_json,
            "extra_data_json": self.extra_data_json,
        }
        if exclude_id:
            d.pop("id", None)
        if exclude_none:
            d = {k: v for k, v in d.items() if v is not None}
        return d

    @property
    def severity_enum(self) -> Severity:
        """Retourne la severity comme enum."""
        return Severity.from_str(self.severity)

    @property
    def error_type_enum(self) -> ErrorType:
        """Retourne le error_type comme enum."""
        return ErrorType.from_str(self.error_type)

    @property
    def related_commits(self) -> list[str]:
        """Parse related_commits_json."""
        return _parse_json(self.related_commits_json) or []

    @related_commits.setter
    def related_commits(self, value: list[str]) -> None:
        """Sérialise related_commits en JSON."""
        self.related_commits_json = _to_json(value)

    @property
    def tags(self) -> list[str]:
        """Parse tags_json."""
        return _parse_json(self.tags_json) or []

    @tags.setter
    def tags(self, value: list[str]) -> None:
        """Sérialise tags en JSON."""
        self.tags_json = _to_json(value)

    @property
    def extra_data(self) -> dict[str, Any]:
        """Parse extra_data_json."""
        return _parse_json(self.extra_data_json) or {}

    @extra_data.setter
    def extra_data(self, value: dict[str, Any]) -> None:
        """Sérialise extra_data en JSON."""
        self.extra_data_json = _to_json(value)


@dataclass(slots=True)
class PipelineRun:
    """
    Représente un run du pipeline d'analyse.
    Correspond à la table `pipeline_runs`.
    """
    id: Optional[int] = None
    run_id: str = ""

    # Contexte Git
    commit_hash: str = ""
    commit_message: Optional[str] = None
    commit_author: Optional[str] = None
    branch_source: Optional[str] = None
    branch_target: Optional[str] = None
    merge_type: Optional[str] = None

    # Contexte JIRA
    jira_key: Optional[str] = None
    jira_type: Optional[str] = None
    jira_summary: Optional[str] = None

    # Résultats
    status: str = "pending"
    overall_score: Optional[int] = None
    recommendation: Optional[str] = None

    # Scores par agent
    score_analyzer: Optional[int] = None
    score_security: Optional[int] = None
    score_reviewer: Optional[int] = None
    score_risk: Optional[int] = None

    # Issues
    issues_critical: int = 0
    issues_high: int = 0
    issues_medium: int = 0
    issues_low: int = 0
    issues_json: Optional[str] = None

    # Fichiers
    files_analyzed: Optional[int] = None
    files_json: Optional[str] = None

    # Rapports
    report_path: Optional[str] = None
    report_json_path: Optional[str] = None
    context_path: Optional[str] = None

    # Timing
    started_at: str = ""
    completed_at: Optional[str] = None
    duration_ms: Optional[int] = None

    # Métadonnées
    trigger: Optional[str] = None
    pipeline_version: Optional[str] = None
    agents_used_json: Optional[str] = None

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "PipelineRun":
        """Crée une instance depuis une ligne SQLite."""
        return cls(
            id=row.get("id"),
            run_id=row.get("run_id", ""),
            commit_hash=row.get("commit_hash", ""),
            commit_message=row.get("commit_message"),
            commit_author=row.get("commit_author"),
            branch_source=row.get("branch_source"),
            branch_target=row.get("branch_target"),
            merge_type=row.get("merge_type"),
            jira_key=row.get("jira_key"),
            jira_type=row.get("jira_type"),
            jira_summary=row.get("jira_summary"),
            status=row.get("status", "pending"),
            overall_score=row.get("overall_score"),
            recommendation=row.get("recommendation"),
            score_analyzer=row.get("score_analyzer"),
            score_security=row.get("score_security"),
            score_reviewer=row.get("score_reviewer"),
            score_risk=row.get("score_risk"),
            issues_critical=row.get("issues_critical", 0) or 0,
            issues_high=row.get("issues_high", 0) or 0,
            issues_medium=row.get("issues_medium", 0) or 0,
            issues_low=row.get("issues_low", 0) or 0,
            issues_json=row.get("issues_json"),
            files_analyzed=row.get("files_analyzed"),
            files_json=row.get("files_json"),
            report_path=row.get("report_path"),
            report_json_path=row.get("report_json_path"),
            context_path=row.get("context_path"),
            started_at=row.get("started_at", ""),
            completed_at=row.get("completed_at"),
            duration_ms=row.get("duration_ms"),
            trigger=row.get("trigger"),
            pipeline_version=row.get("pipeline_version"),
            agents_used_json=row.get("agents_used_json"),
        )

    def to_dict(self, exclude_none: bool = False, exclude_id: bool = False) -> dict[str, Any]:
        """Convertit en dictionnaire pour insertion SQL."""
        d = {
            "id": self.id,
            "run_id": self.run_id,
            "commit_hash": self.commit_hash,
            "commit_message": self.commit_message,
            "commit_author": self.commit_author,
            "branch_source": self.branch_source,
            "branch_target": self.branch_target,
            "merge_type": self.merge_type,
            "jira_key": self.jira_key,
            "jira_type": self.jira_type,
            "jira_summary": self.jira_summary,
            "status": self.status,
            "overall_score": self.overall_score,
            "recommendation": self.recommendation,
            "score_analyzer": self.score_analyzer,
            "score_security": self.score_security,
            "score_reviewer": self.score_reviewer,
            "score_risk": self.score_risk,
            "issues_critical": self.issues_critical,
            "issues_high": self.issues_high,
            "issues_medium": self.issues_medium,
            "issues_low": self.issues_low,
            "issues_json": self.issues_json,
            "files_analyzed": self.files_analyzed,
            "files_json": self.files_json,
            "report_path": self.report_path,
            "report_json_path": self.report_json_path,
            "context_path": self.context_path,
            "started_at": self.started_at,
            "completed_at": self.completed_at,
            "duration_ms": self.duration_ms,
            "trigger": self.trigger,
            "pipeline_version": self.pipeline_version,
            "agents_used_json": self.agents_used_json,
        }
        if exclude_id:
            d.pop("id", None)
        if exclude_none:
            d = {k: v for k, v in d.items() if v is not None}
        return d

    @property
    def status_enum(self) -> PipelineStatus:
        """Retourne le status comme enum."""
        return PipelineStatus.from_str(self.status)

    @property
    def issues(self) -> list[dict[str, Any]]:
        """Parse issues_json."""
        return _parse_json(self.issues_json) or []

    @issues.setter
    def issues(self, value: list[dict[str, Any]]) -> None:
        """Sérialise issues en JSON."""
        self.issues_json = _to_json(value)

    @property
    def files(self) -> list[str]:
        """Parse files_json."""
        return _parse_json(self.files_json) or []

    @files.setter
    def files(self, value: list[str]) -> None:
        """Sérialise files en JSON."""
        self.files_json = _to_json(value)

    @property
    def agents_used(self) -> list[str]:
        """Parse agents_used_json."""
        return _parse_json(self.agents_used_json) or []

    @agents_used.setter
    def agents_used(self, value: list[str]) -> None:
        """Sérialise agents_used en JSON."""
        self.agents_used_json = _to_json(value)

    @property
    def total_issues(self) -> int:
        """Compte total des issues."""
        return self.issues_critical + self.issues_high + self.issues_medium + self.issues_low


@dataclass(slots=True)
class SnapshotSymbol:
    """
    Snapshot temporaire d'un symbole pour comparaison.
    Correspond à la table `snapshot_symbols`.
    """
    id: Optional[int] = None
    run_id: str = ""
    file_path: str = ""
    symbol_name: str = ""
    symbol_kind: str = ""
    signature: Optional[str] = None
    complexity: Optional[int] = None
    line_start: Optional[int] = None
    line_end: Optional[int] = None
    hash: Optional[str] = None
    created_at: Optional[str] = None

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "SnapshotSymbol":
        """Crée une instance depuis une ligne SQLite."""
        return cls(
            id=row.get("id"),
            run_id=row.get("run_id", ""),
            file_path=row.get("file_path", ""),
            symbol_name=row.get("symbol_name", ""),
            symbol_kind=row.get("symbol_kind", ""),
            signature=row.get("signature"),
            complexity=row.get("complexity"),
            line_start=row.get("line_start"),
            line_end=row.get("line_end"),
            hash=row.get("hash"),
            created_at=row.get("created_at"),
        )

    def to_dict(self, exclude_none: bool = False, exclude_id: bool = False) -> dict[str, Any]:
        """Convertit en dictionnaire pour insertion SQL."""
        d = {
            "id": self.id,
            "run_id": self.run_id,
            "file_path": self.file_path,
            "symbol_name": self.symbol_name,
            "symbol_kind": self.symbol_kind,
            "signature": self.signature,
            "complexity": self.complexity,
            "line_start": self.line_start,
            "line_end": self.line_end,
            "hash": self.hash,
            "created_at": self.created_at,
        }
        if exclude_id:
            d.pop("id", None)
        if exclude_none:
            d = {k: v for k, v in d.items() if v is not None}
        return d


# =============================================================================
# PILIER 3 : BASE DE CONNAISSANCES
# =============================================================================

@dataclass(slots=True)
class Pattern:
    """
    Représente un pattern de code à respecter.
    Correspond à la table `patterns`.
    """
    id: Optional[int] = None
    name: str = ""
    category: str = ""

    # Scope
    scope: str = "project"
    module: Optional[str] = None
    file_pattern: Optional[str] = None

    # Description
    title: str = ""
    description: str = ""
    rationale: Optional[str] = None

    # Exemples
    good_example: Optional[str] = None
    bad_example: Optional[str] = None
    explanation: Optional[str] = None

    # Règles
    rules_json: Optional[str] = None

    # Métadonnées
    severity: str = "warning"
    is_active: bool = True
    created_at: Optional[str] = None
    updated_at: Optional[str] = None
    created_by: Optional[str] = None

    # Références
    related_adr: Optional[str] = None
    external_link: Optional[str] = None
    examples_in_code_json: Optional[str] = None

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "Pattern":
        """Crée une instance depuis une ligne SQLite."""
        return cls(
            id=row.get("id"),
            name=row.get("name", ""),
            category=row.get("category", ""),
            scope=row.get("scope", "project"),
            module=row.get("module"),
            file_pattern=row.get("file_pattern"),
            title=row.get("title", ""),
            description=row.get("description", ""),
            rationale=row.get("rationale"),
            good_example=row.get("good_example"),
            bad_example=row.get("bad_example"),
            explanation=row.get("explanation"),
            rules_json=row.get("rules_json"),
            severity=row.get("severity", "warning"),
            is_active=bool(row.get("is_active", 1)),
            created_at=row.get("created_at"),
            updated_at=row.get("updated_at"),
            created_by=row.get("created_by"),
            related_adr=row.get("related_adr"),
            external_link=row.get("external_link"),
            examples_in_code_json=row.get("examples_in_code_json"),
        )

    def to_dict(self, exclude_none: bool = False, exclude_id: bool = False) -> dict[str, Any]:
        """Convertit en dictionnaire pour insertion SQL."""
        d = {
            "id": self.id,
            "name": self.name,
            "category": self.category,
            "scope": self.scope,
            "module": self.module,
            "file_pattern": self.file_pattern,
            "title": self.title,
            "description": self.description,
            "rationale": self.rationale,
            "good_example": self.good_example,
            "bad_example": self.bad_example,
            "explanation": self.explanation,
            "rules_json": self.rules_json,
            "severity": self.severity,
            "is_active": int(self.is_active),
            "created_at": self.created_at,
            "updated_at": self.updated_at,
            "created_by": self.created_by,
            "related_adr": self.related_adr,
            "external_link": self.external_link,
            "examples_in_code_json": self.examples_in_code_json,
        }
        if exclude_id:
            d.pop("id", None)
        if exclude_none:
            d = {k: v for k, v in d.items() if v is not None}
        return d

    @property
    def severity_enum(self) -> Severity:
        """Retourne la severity comme enum."""
        return Severity.from_str(self.severity)

    @property
    def rules(self) -> list[dict[str, Any]]:
        """Parse rules_json."""
        return _parse_json(self.rules_json) or []

    @rules.setter
    def rules(self, value: list[dict[str, Any]]) -> None:
        """Sérialise rules en JSON."""
        self.rules_json = _to_json(value)

    @property
    def examples_in_code(self) -> list[str]:
        """Parse examples_in_code_json."""
        return _parse_json(self.examples_in_code_json) or []

    @examples_in_code.setter
    def examples_in_code(self, value: list[str]) -> None:
        """Sérialise examples_in_code en JSON."""
        self.examples_in_code_json = _to_json(value)


@dataclass(slots=True)
class ArchitectureDecision:
    """
    Représente une décision architecturale (ADR).
    Correspond à la table `architecture_decisions`.
    """
    id: Optional[int] = None
    decision_id: str = ""

    # Statut
    status: str = "proposed"
    superseded_by: Optional[str] = None

    # Contenu
    title: str = ""
    context: str = ""
    decision: str = ""
    consequences: Optional[str] = None
    alternatives: Optional[str] = None

    # Scope
    affected_modules_json: Optional[str] = None
    affected_files_json: Optional[str] = None

    # Métadonnées
    date_proposed: Optional[str] = None
    date_decided: Optional[str] = None
    decided_by: Optional[str] = None
    stakeholders_json: Optional[str] = None

    # Liens
    related_adrs_json: Optional[str] = None
    jira_tickets_json: Optional[str] = None
    documentation_link: Optional[str] = None

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "ArchitectureDecision":
        """Crée une instance depuis une ligne SQLite."""
        return cls(
            id=row.get("id"),
            decision_id=row.get("decision_id", ""),
            status=row.get("status", "proposed"),
            superseded_by=row.get("superseded_by"),
            title=row.get("title", ""),
            context=row.get("context", ""),
            decision=row.get("decision", ""),
            consequences=row.get("consequences"),
            alternatives=row.get("alternatives"),
            affected_modules_json=row.get("affected_modules_json"),
            affected_files_json=row.get("affected_files_json"),
            date_proposed=row.get("date_proposed"),
            date_decided=row.get("date_decided"),
            decided_by=row.get("decided_by"),
            stakeholders_json=row.get("stakeholders_json"),
            related_adrs_json=row.get("related_adrs_json"),
            jira_tickets_json=row.get("jira_tickets_json"),
            documentation_link=row.get("documentation_link"),
        )

    def to_dict(self, exclude_none: bool = False, exclude_id: bool = False) -> dict[str, Any]:
        """Convertit en dictionnaire pour insertion SQL."""
        d = {
            "id": self.id,
            "decision_id": self.decision_id,
            "status": self.status,
            "superseded_by": self.superseded_by,
            "title": self.title,
            "context": self.context,
            "decision": self.decision,
            "consequences": self.consequences,
            "alternatives": self.alternatives,
            "affected_modules_json": self.affected_modules_json,
            "affected_files_json": self.affected_files_json,
            "date_proposed": self.date_proposed,
            "date_decided": self.date_decided,
            "decided_by": self.decided_by,
            "stakeholders_json": self.stakeholders_json,
            "related_adrs_json": self.related_adrs_json,
            "jira_tickets_json": self.jira_tickets_json,
            "documentation_link": self.documentation_link,
        }
        if exclude_id:
            d.pop("id", None)
        if exclude_none:
            d = {k: v for k, v in d.items() if v is not None}
        return d

    @property
    def status_enum(self) -> ADRStatus:
        """Retourne le status comme enum."""
        return ADRStatus.from_str(self.status)

    @property
    def affected_modules(self) -> list[str]:
        """Parse affected_modules_json."""
        return _parse_json(self.affected_modules_json) or []

    @affected_modules.setter
    def affected_modules(self, value: list[str]) -> None:
        """Sérialise affected_modules en JSON."""
        self.affected_modules_json = _to_json(value)

    @property
    def affected_files(self) -> list[str]:
        """Parse affected_files_json."""
        return _parse_json(self.affected_files_json) or []

    @affected_files.setter
    def affected_files(self, value: list[str]) -> None:
        """Sérialise affected_files en JSON."""
        self.affected_files_json = _to_json(value)

    @property
    def stakeholders(self) -> list[str]:
        """Parse stakeholders_json."""
        return _parse_json(self.stakeholders_json) or []

    @stakeholders.setter
    def stakeholders(self, value: list[str]) -> None:
        """Sérialise stakeholders en JSON."""
        self.stakeholders_json = _to_json(value)

    @property
    def related_adrs(self) -> list[str]:
        """Parse related_adrs_json."""
        return _parse_json(self.related_adrs_json) or []

    @related_adrs.setter
    def related_adrs(self, value: list[str]) -> None:
        """Sérialise related_adrs en JSON."""
        self.related_adrs_json = _to_json(value)

    @property
    def jira_tickets(self) -> list[str]:
        """Parse jira_tickets_json."""
        return _parse_json(self.jira_tickets_json) or []

    @jira_tickets.setter
    def jira_tickets(self, value: list[str]) -> None:
        """Sérialise jira_tickets en JSON."""
        self.jira_tickets_json = _to_json(value)


@dataclass(slots=True)
class CriticalPath:
    """
    Représente un chemin marqué comme critique.
    Correspond à la table `critical_paths`.
    """
    id: Optional[int] = None
    pattern: str = ""
    reason: str = ""
    severity: str = "high"
    added_by: Optional[str] = None
    added_at: Optional[str] = None

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "CriticalPath":
        """Crée une instance depuis une ligne SQLite."""
        return cls(
            id=row.get("id"),
            pattern=row.get("pattern", ""),
            reason=row.get("reason", ""),
            severity=row.get("severity", "high"),
            added_by=row.get("added_by"),
            added_at=row.get("added_at"),
        )

    def to_dict(self, exclude_none: bool = False, exclude_id: bool = False) -> dict[str, Any]:
        """Convertit en dictionnaire pour insertion SQL."""
        d = {
            "id": self.id,
            "pattern": self.pattern,
            "reason": self.reason,
            "severity": self.severity,
            "added_by": self.added_by,
            "added_at": self.added_at,
        }
        if exclude_id:
            d.pop("id", None)
        if exclude_none:
            d = {k: v for k, v in d.items() if v is not None}
        return d

    @property
    def severity_enum(self) -> Severity:
        """Retourne la severity comme enum."""
        return Severity.from_str(self.severity)


# =============================================================================
# MODÈLES COMPOSITES (pour les résultats de requêtes complexes)
# =============================================================================

@dataclass(slots=True)
class SymbolWithContext:
    """
    Symbole avec son contexte fichier.
    Utilisé par la vue v_symbols_with_context.
    """
    # Champs de Symbol
    symbol: Symbol
    # Champs ajoutés du fichier
    file_path: str = ""
    file_module: Optional[str] = None
    file_is_critical: bool = False
    file_language: Optional[str] = None

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "SymbolWithContext":
        """Crée une instance depuis une ligne de la vue."""
        return cls(
            symbol=Symbol.from_row(row),
            file_path=row.get("file_path", ""),
            file_module=row.get("file_module"),
            file_is_critical=bool(row.get("file_is_critical", 0)),
            file_language=row.get("file_language"),
        )


@dataclass(slots=True)
class RelationNamed:
    """
    Relation avec les noms des symboles.
    Utilisé par la vue v_relations_named.
    """
    id: int = 0
    relation_type: str = ""
    count: int = 1
    location_line: Optional[int] = None
    source_name: str = ""
    source_kind: str = ""
    source_file: str = ""
    target_name: str = ""
    target_kind: str = ""
    target_file: str = ""

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "RelationNamed":
        """Crée une instance depuis une ligne de la vue."""
        return cls(
            id=row.get("id", 0),
            relation_type=row.get("relation_type", ""),
            count=row.get("count", 1) or 1,
            location_line=row.get("location_line"),
            source_name=row.get("source_name", ""),
            source_kind=row.get("source_kind", ""),
            source_file=row.get("source_file", ""),
            target_name=row.get("target_name", ""),
            target_kind=row.get("target_kind", ""),
            target_file=row.get("target_file", ""),
        )


@dataclass(slots=True)
class CallerInfo:
    """
    Information sur un appelant d'un symbole.
    Utilisé pour les résultats de get_symbol_callers.
    """
    id: int = 0
    name: str = ""
    kind: str = ""
    file_path: str = ""
    line: Optional[int] = None
    depth: int = 0
    is_direct: bool = True
    is_critical: bool = False

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "CallerInfo":
        """Crée une instance depuis une ligne."""
        return cls(
            id=row.get("id", 0),
            name=row.get("name", ""),
            kind=row.get("kind", ""),
            file_path=row.get("file_path", ""),
            line=row.get("location_line") or row.get("line"),
            depth=row.get("depth", 0),
            is_direct=bool(row.get("is_direct", 1)),
            is_critical=bool(row.get("is_critical", 0)),
        )


@dataclass(slots=True)
class FileWithStats:
    """
    Fichier avec statistiques de symboles.
    Utilisé par la vue v_files_with_stats.
    """
    file: File
    symbol_count: int = 0
    function_count: int = 0
    type_count: int = 0
    avg_complexity: float = 0.0

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "FileWithStats":
        """Crée une instance depuis une ligne de la vue."""
        return cls(
            file=File.from_row(row),
            symbol_count=row.get("symbol_count", 0) or 0,
            function_count=row.get("function_count", 0) or 0,
            type_count=row.get("type_count", 0) or 0,
            avg_complexity=row.get("avg_complexity", 0.0) or 0.0,
        )


@dataclass(slots=True)
class HighRiskFile:
    """
    Fichier à haut risque.
    Utilisé par la vue v_high_risk_files.
    """
    id: int = 0
    path: str = ""
    module: Optional[str] = None
    is_critical: bool = False
    complexity_avg: float = 0.0
    error_count: int = 0
    max_severity: Optional[str] = None

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "HighRiskFile":
        """Crée une instance depuis une ligne de la vue."""
        return cls(
            id=row.get("id", 0),
            path=row.get("path", ""),
            module=row.get("module"),
            is_critical=bool(row.get("is_critical", 0)),
            complexity_avg=row.get("complexity_avg", 0.0) or 0.0,
            error_count=row.get("error_count", 0) or 0,
            max_severity=row.get("max_severity"),
        )

    @property
    def max_severity_enum(self) -> Optional[Severity]:
        """Retourne max_severity comme enum."""
        if self.max_severity:
            return Severity.from_str(self.max_severity)
        return None


@dataclass(slots=True)
class ImpactAnalysis:
    """
    Résultat d'une analyse d'impact.

    Contient les fichiers/symboles affectés par une modification,
    organisés par type d'impact.
    """
    file_path: str = ""
    direct_impact: list[dict[str, Any]] = field(default_factory=list)
    transitive_impact: list[dict[str, Any]] = field(default_factory=list)
    include_impact: list[dict[str, Any]] = field(default_factory=list)
    summary: dict[str, Any] = field(default_factory=dict)

    def __post_init__(self) -> None:
        """Workaround for slots=True with mutable defaults."""
        if not isinstance(self.direct_impact, list):
            object.__setattr__(self, 'direct_impact', list(self.direct_impact) if self.direct_impact else [])
        if not isinstance(self.transitive_impact, list):
            object.__setattr__(self, 'transitive_impact', list(self.transitive_impact) if self.transitive_impact else [])
        if not isinstance(self.include_impact, list):
            object.__setattr__(self, 'include_impact', list(self.include_impact) if self.include_impact else [])
        if not isinstance(self.summary, dict):
            object.__setattr__(self, 'summary', dict(self.summary) if self.summary else {})

    @classmethod
    def from_row(cls, row: dict[str, Any]) -> "ImpactAnalysis":
        """Crée une instance depuis un dict."""
        return cls(
            file_path=row.get("file_path", ""),
            direct_impact=row.get("direct_impact", []) or [],
            transitive_impact=row.get("transitive_impact", []) or [],
            include_impact=row.get("include_impact", []) or [],
            summary=row.get("summary", {}) or {},
        )

    def to_dict(self) -> dict[str, Any]:
        """Convertit en dictionnaire."""
        return {
            "file_path": self.file_path,
            "direct_impact": self.direct_impact,
            "transitive_impact": self.transitive_impact,
            "include_impact": self.include_impact,
            "summary": self.summary,
        }

    @property
    def total_impact_count(self) -> int:
        """Nombre total de fichiers/symboles impactés."""
        return len(self.direct_impact) + len(self.transitive_impact) + len(self.include_impact)

    @property
    def has_critical_impact(self) -> bool:
        """Vérifie s'il y a un impact sur des éléments critiques."""
        for item in self.direct_impact + self.transitive_impact:
            if item.get("is_critical", False):
                return True
        return False
