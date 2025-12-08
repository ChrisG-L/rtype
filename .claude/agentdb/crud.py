"""
AgentDB - Opérations CRUD pour chaque table.

Ce module fournit les opérations Create, Read, Update, Delete pour :
- Pilier 1 : files, symbols, relations, file_relations
- Pilier 2 : error_history, pipeline_runs, snapshot_symbols
- Pilier 3 : patterns, architecture_decisions, critical_paths

Chaque repository utilise les dataclasses de models.py et la classe
Database de db.py.

Usage:
    from agentdb.db import DatabaseManager
    from agentdb.crud import FileRepository, SymbolRepository

    with DatabaseManager("db.sqlite") as db:
        files = FileRepository(db)
        symbols = SymbolRepository(db)

        # Insérer un fichier
        file_id = files.insert(File(path="src/main.c", ...))

        # Récupérer par chemin
        file = files.get_by_path("src/main.c")

        # Mettre à jour
        files.update(file_id, lines_total=500)
"""

from __future__ import annotations

import fnmatch
import logging
from datetime import datetime, timedelta
from typing import Any, Optional, Union

from .db import Database
from .models import (
    ArchitectureDecision,
    CriticalPath,
    ErrorHistory,
    File,
    FileRelation,
    Pattern,
    PipelineRun,
    Relation,
    SnapshotSymbol,
    Symbol,
)

logger = logging.getLogger("agentdb.crud")


# =============================================================================
# BASE REPOSITORY
# =============================================================================

class BaseRepository:
    """
    Classe de base pour tous les repositories.

    Fournit les méthodes communes et la gestion de la connexion.
    """

    def __init__(self, db: Database) -> None:
        """
        Initialise le repository.

        Args:
            db: Instance de Database connectée
        """
        self.db = db

    def _build_update_sql(
        self,
        table: str,
        id_column: str,
        **kwargs: Any,
    ) -> tuple[str, list[Any]]:
        """
        Construit une requête UPDATE à partir de kwargs.

        Args:
            table: Nom de la table
            id_column: Nom de la colonne ID
            **kwargs: Colonnes à mettre à jour

        Returns:
            Tuple (sql, params)
        """
        if not kwargs:
            raise ValueError("No columns to update")

        columns = []
        values = []
        for key, value in kwargs.items():
            if key != id_column:
                columns.append(f"{key} = ?")
                values.append(value)

        sql = f"UPDATE {table} SET {', '.join(columns)} WHERE {id_column} = ?"
        return sql, values


# =============================================================================
# FILE REPOSITORY
# =============================================================================

class FileRepository(BaseRepository):
    """
    Repository pour la table `files`.

    Gère les opérations CRUD sur les fichiers du projet.
    """

    TABLE = "files"

    def insert(self, file: File) -> int:
        """
        Insère un nouveau fichier.

        Args:
            file: Instance de File à insérer

        Returns:
            ID du fichier créé

        Raises:
            DatabaseError: Si l'insertion échoue
        """
        data = file.to_dict(exclude_id=True)

        # Set indexed_at to current timestamp if not provided
        if not data.get("indexed_at"):
            data["indexed_at"] = datetime.now().isoformat()

        columns = list(data.keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        cursor = self.db.execute(sql, tuple(data.values()))
        file_id = cursor.lastrowid
        cursor.close()

        logger.debug(f"Inserted file: {file.path} (id={file_id})")
        return file_id

    def insert_or_update(self, file: File) -> int:
        """
        Insère ou met à jour un fichier (upsert).

        Si le fichier existe (même path), le met à jour.
        Sinon, l'insère.

        Args:
            file: Instance de File

        Returns:
            ID du fichier
        """
        existing = self.get_by_path(file.path)
        if existing:
            self.update(existing.id, **file.to_dict(exclude_id=True))
            return existing.id
        return self.insert(file)

    def get_by_id(self, file_id: int) -> Optional[File]:
        """
        Récupère un fichier par son ID.

        Args:
            file_id: ID du fichier

        Returns:
            Instance de File ou None si non trouvé
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE id = ?",
            (file_id,),
        )
        if row:
            return File.from_row(row)
        return None

    def get_by_path(self, path: str) -> Optional[File]:
        """
        Récupère un fichier par son chemin.

        Args:
            path: Chemin du fichier (relatif à la racine du projet)

        Returns:
            Instance de File ou None si non trouvé
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE path = ?",
            (path,),
        )
        if row:
            return File.from_row(row)
        return None

    def get_all(self, limit: int = 1000, offset: int = 0) -> list[File]:
        """
        Récupère tous les fichiers avec pagination.

        Args:
            limit: Nombre maximum de résultats
            offset: Décalage pour la pagination

        Returns:
            Liste de File
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} ORDER BY path LIMIT ? OFFSET ?",
            (limit, offset),
        )
        return [File.from_row(row) for row in rows]

    def get_by_module(self, module: str) -> list[File]:
        """
        Récupère tous les fichiers d'un module.

        Args:
            module: Nom du module

        Returns:
            Liste de File du module
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} WHERE module = ? ORDER BY path",
            (module,),
        )
        return [File.from_row(row) for row in rows]

    def get_by_extension(self, extension: str) -> list[File]:
        """
        Récupère tous les fichiers avec une extension donnée.

        Args:
            extension: Extension (ex: ".c", ".py")

        Returns:
            Liste de File
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} WHERE extension = ? ORDER BY path",
            (extension,),
        )
        return [File.from_row(row) for row in rows]

    def get_by_language(self, language: str) -> list[File]:
        """
        Récupère tous les fichiers d'un langage.

        Args:
            language: Langage (ex: "c", "python")

        Returns:
            Liste de File
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} WHERE language = ? ORDER BY path",
            (language,),
        )
        return [File.from_row(row) for row in rows]

    def get_critical_files(self) -> list[File]:
        """
        Récupère tous les fichiers marqués comme critiques.

        Returns:
            Liste de File critiques
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} WHERE is_critical = 1 ORDER BY path"
        )
        return [File.from_row(row) for row in rows]

    def get_security_sensitive_files(self) -> list[File]:
        """
        Récupère tous les fichiers sensibles à la sécurité.

        Returns:
            Liste de File sensibles
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} WHERE security_sensitive = 1 ORDER BY path"
        )
        return [File.from_row(row) for row in rows]

    def get_recently_modified(self, days: int = 30) -> list[File]:
        """
        Récupère les fichiers récemment modifiés.

        Args:
            days: Nombre de jours en arrière

        Returns:
            Liste de File
        """
        cutoff = (datetime.now() - timedelta(days=days)).isoformat()
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE last_modified >= ?
            ORDER BY last_modified DESC
            """,
            (cutoff,),
        )
        return [File.from_row(row) for row in rows]

    def search_by_pattern(self, pattern: str) -> list[File]:
        """
        Recherche des fichiers par pattern glob.

        Args:
            pattern: Pattern glob (ex: "src/**/*.c", "*.py")

        Returns:
            Liste de File correspondants
        """
        sql_pattern = pattern.replace("**", "%").replace("*", "%").replace("?", "_")
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} WHERE path LIKE ? ORDER BY path",
            (sql_pattern,),
        )
        files = [File.from_row(row) for row in rows]
        return [f for f in files if fnmatch.fnmatch(f.path, pattern)]

    def update(self, file_id: int, **kwargs: Any) -> bool:
        """
        Met à jour un fichier.

        Args:
            file_id: ID du fichier
            **kwargs: Colonnes à mettre à jour

        Returns:
            True si mis à jour, False sinon
        """
        if not kwargs:
            return False

        sql, values = self._build_update_sql(self.TABLE, "id", **kwargs)
        values.append(file_id)

        cursor = self.db.execute(sql, tuple(values))
        updated = cursor.rowcount > 0
        cursor.close()

        if updated:
            logger.debug(f"Updated file id={file_id}")
        return updated

    def delete(self, file_id: int) -> bool:
        """
        Supprime un fichier.

        Note: Cascade les suppressions sur symbols et relations.

        Args:
            file_id: ID du fichier

        Returns:
            True si supprimé, False sinon
        """
        cursor = self.db.execute(
            f"DELETE FROM {self.TABLE} WHERE id = ?",
            (file_id,),
        )
        deleted = cursor.rowcount > 0
        cursor.close()

        if deleted:
            logger.debug(f"Deleted file id={file_id}")
        return deleted

    def delete_by_path(self, path: str) -> bool:
        """
        Supprime un fichier par son chemin.

        Args:
            path: Chemin du fichier

        Returns:
            True si supprimé, False sinon
        """
        cursor = self.db.execute(
            f"DELETE FROM {self.TABLE} WHERE path = ?",
            (path,),
        )
        deleted = cursor.rowcount > 0
        cursor.close()
        return deleted

    def count(self) -> int:
        """
        Compte le nombre total de fichiers.

        Returns:
            Nombre de fichiers
        """
        return self.db.fetch_scalar(f"SELECT COUNT(*) FROM {self.TABLE}") or 0

    def count_by_module(self) -> dict[str, int]:
        """
        Compte les fichiers par module.

        Returns:
            Dictionnaire {module: count}
        """
        rows = self.db.fetch_all(
            f"""
            SELECT module, COUNT(*) as count
            FROM {self.TABLE}
            WHERE module IS NOT NULL
            GROUP BY module
            ORDER BY count DESC
            """
        )
        return {row["module"]: row["count"] for row in rows}

    def get_modules(self) -> list[str]:
        """
        Liste tous les modules distincts.

        Returns:
            Liste des noms de modules
        """
        rows = self.db.fetch_all(
            f"""
            SELECT DISTINCT module
            FROM {self.TABLE}
            WHERE module IS NOT NULL
            ORDER BY module
            """
        )
        return [row["module"] for row in rows]


# =============================================================================
# SYMBOL REPOSITORY
# =============================================================================

class SymbolRepository(BaseRepository):
    """
    Repository pour la table `symbols`.

    Gère les opérations CRUD sur les symboles (fonctions, types, etc.).
    """

    TABLE = "symbols"

    def insert(self, symbol: Symbol) -> int:
        """
        Insère un nouveau symbole.

        Args:
            symbol: Instance de Symbol à insérer

        Returns:
            ID du symbole créé
        """
        data = symbol.to_dict(exclude_id=True)

        # Set indexed_at to current timestamp if not provided
        if not data.get("indexed_at"):
            data["indexed_at"] = datetime.now().isoformat()

        columns = list(data.keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        cursor = self.db.execute(sql, tuple(data.values()))
        symbol_id = cursor.lastrowid
        cursor.close()

        logger.debug(f"Inserted symbol: {symbol.name} (id={symbol_id})")
        return symbol_id

    def insert_many(self, symbols: list[Symbol]) -> int:
        """
        Insère plusieurs symboles en batch.

        Args:
            symbols: Liste de Symbol à insérer

        Returns:
            Nombre de symboles insérés
        """
        if not symbols:
            return 0

        now = datetime.now().isoformat()
        params = []
        for s in symbols:
            data = s.to_dict(exclude_id=True)
            if not data.get("indexed_at"):
                data["indexed_at"] = now
            params.append(tuple(data.values()))

        columns = list(symbols[0].to_dict(exclude_id=True).keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        count = self.db.execute_many(sql, params)

        logger.debug(f"Inserted {count} symbols")
        return count

    def get_by_id(self, symbol_id: int) -> Optional[Symbol]:
        """
        Récupère un symbole par son ID.

        Args:
            symbol_id: ID du symbole

        Returns:
            Instance de Symbol ou None
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE id = ?",
            (symbol_id,),
        )
        if row:
            return Symbol.from_row(row)
        return None

    def get_by_file(self, file_id: int) -> list[Symbol]:
        """
        Récupère tous les symboles d'un fichier.

        Args:
            file_id: ID du fichier

        Returns:
            Liste de Symbol du fichier
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} WHERE file_id = ? ORDER BY line_start",
            (file_id,),
        )
        return [Symbol.from_row(row) for row in rows]

    def get_by_name(
        self,
        name: str,
        file_path: Optional[str] = None,
        kind: Optional[str] = None,
    ) -> Optional[Symbol]:
        """
        Récupère un symbole par son nom.

        Args:
            name: Nom du symbole
            file_path: Chemin du fichier (optionnel, pour désambiguïser)
            kind: Type de symbole (optionnel)

        Returns:
            Instance de Symbol ou None
        """
        if file_path:
            sql = f"""
                SELECT s.* FROM {self.TABLE} s
                JOIN files f ON s.file_id = f.id
                WHERE s.name = ? AND f.path = ?
            """
            params: tuple = (name, file_path)
            if kind:
                sql += " AND s.kind = ?"
                params = (name, file_path, kind)
        else:
            sql = f"SELECT * FROM {self.TABLE} WHERE name = ?"
            params = (name,)
            if kind:
                sql += " AND kind = ?"
                params = (name, kind)

        row = self.db.fetch_one(sql, params)
        if row:
            return Symbol.from_row(row)
        return None

    def get_by_name_all(
        self,
        name: str,
        kind: Optional[str] = None,
    ) -> list[Symbol]:
        """
        Récupère tous les symboles avec un nom donné.

        Args:
            name: Nom du symbole
            kind: Type de symbole (optionnel)

        Returns:
            Liste de Symbol
        """
        if kind:
            rows = self.db.fetch_all(
                f"SELECT * FROM {self.TABLE} WHERE name = ? AND kind = ?",
                (name, kind),
            )
        else:
            rows = self.db.fetch_all(
                f"SELECT * FROM {self.TABLE} WHERE name = ?",
                (name,),
            )
        return [Symbol.from_row(row) for row in rows]

    def get_by_name_and_file(self, name: str, file_id: int) -> Optional[Symbol]:
        """
        Récupère un symbole unique par nom et fichier.

        Args:
            name: Nom du symbole
            file_id: ID du fichier

        Returns:
            Symbol ou None
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE name = ? AND file_id = ?",
            (name, file_id),
        )
        if row:
            return Symbol.from_row(row)
        return None

    def search(
        self,
        query: str,
        kind: Optional[str] = None,
        module: Optional[str] = None,
        limit: int = 50,
    ) -> list[Symbol]:
        """
        Recherche des symboles par pattern.

        Args:
            query: Pattern de recherche (supporte * et ?)
            kind: Type de symbole (optionnel)
            module: Module à filtrer (optionnel)
            limit: Nombre maximum de résultats

        Returns:
            Liste de Symbol correspondants
        """
        sql_pattern = query.replace("*", "%").replace("?", "_")

        sql = f"SELECT s.* FROM {self.TABLE} s"
        params: list[Any] = []

        if module:
            sql += " JOIN files f ON s.file_id = f.id"

        sql += " WHERE s.name LIKE ?"
        params.append(sql_pattern)

        if kind:
            sql += " AND s.kind = ?"
            params.append(kind)

        if module:
            sql += " AND f.module = ?"
            params.append(module)

        sql += f" ORDER BY s.name LIMIT ?"
        params.append(limit)

        rows = self.db.fetch_all(sql, tuple(params))
        return [Symbol.from_row(row) for row in rows]

    def get_by_kind(self, kind: str, file_id: Optional[int] = None) -> list[Symbol]:
        """
        Récupère les symboles par type.

        Args:
            kind: Type de symbole (function, struct, etc.)
            file_id: Optionnel, filtrer par fichier

        Returns:
            Liste de Symbol
        """
        if file_id:
            rows = self.db.fetch_all(
                f"SELECT * FROM {self.TABLE} WHERE kind = ? AND file_id = ? ORDER BY name",
                (kind, file_id),
            )
        else:
            rows = self.db.fetch_all(
                f"SELECT * FROM {self.TABLE} WHERE kind = ? ORDER BY name",
                (kind,),
            )
        return [Symbol.from_row(row) for row in rows]

    def get_functions_by_file(self, file_id: int) -> list[Symbol]:
        """
        Récupère toutes les fonctions d'un fichier.

        Args:
            file_id: ID du fichier

        Returns:
            Liste de Symbol de type function
        """
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE file_id = ? AND kind IN ('function', 'method')
            ORDER BY line_start
            """,
            (file_id,),
        )
        return [Symbol.from_row(row) for row in rows]

    def get_types_by_file(self, file_id: int) -> list[Symbol]:
        """
        Récupère tous les types définis dans un fichier.

        Args:
            file_id: ID du fichier

        Returns:
            Liste de Symbol de type struct/class/enum/typedef
        """
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE file_id = ? AND kind IN ('struct', 'class', 'enum', 'typedef', 'interface')
            ORDER BY line_start
            """,
            (file_id,),
        )
        return [Symbol.from_row(row) for row in rows]

    def update(self, symbol_id: int, **kwargs: Any) -> bool:
        """
        Met à jour un symbole.

        Args:
            symbol_id: ID du symbole
            **kwargs: Colonnes à mettre à jour

        Returns:
            True si mis à jour
        """
        if not kwargs:
            return False

        sql, values = self._build_update_sql(self.TABLE, "id", **kwargs)
        values.append(symbol_id)

        cursor = self.db.execute(sql, tuple(values))
        updated = cursor.rowcount > 0
        cursor.close()
        return updated

    def delete(self, symbol_id: int) -> bool:
        """
        Supprime un symbole.

        Args:
            symbol_id: ID du symbole

        Returns:
            True si supprimé
        """
        cursor = self.db.execute(
            f"DELETE FROM {self.TABLE} WHERE id = ?",
            (symbol_id,),
        )
        deleted = cursor.rowcount > 0
        cursor.close()
        return deleted

    def delete_by_file(self, file_id: int) -> int:
        """
        Supprime tous les symboles d'un fichier.

        Args:
            file_id: ID du fichier

        Returns:
            Nombre de symboles supprimés
        """
        cursor = self.db.execute(
            f"DELETE FROM {self.TABLE} WHERE file_id = ?",
            (file_id,),
        )
        count = cursor.rowcount
        cursor.close()

        logger.debug(f"Deleted {count} symbols for file_id={file_id}")
        return count

    def count(self) -> int:
        """Compte le nombre total de symboles."""
        return self.db.fetch_scalar(f"SELECT COUNT(*) FROM {self.TABLE}") or 0

    def count_by_kind(self) -> dict[str, int]:
        """Compte les symboles par type."""
        rows = self.db.fetch_all(
            f"""
            SELECT kind, COUNT(*) as count
            FROM {self.TABLE}
            GROUP BY kind
            ORDER BY count DESC
            """
        )
        return {row["kind"]: row["count"] for row in rows}


# =============================================================================
# RELATION REPOSITORY
# =============================================================================

class RelationRepository(BaseRepository):
    """
    Repository pour la table `relations`.

    Gère les relations entre symboles (calls, uses, etc.).
    """

    TABLE = "relations"

    def insert(self, relation: Relation) -> int:
        """
        Insère une nouvelle relation.

        Args:
            relation: Instance de Relation à insérer

        Returns:
            ID de la relation créée
        """
        data = relation.to_dict(exclude_id=True)
        columns = list(data.keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        cursor = self.db.execute(sql, tuple(data.values()))
        relation_id = cursor.lastrowid
        cursor.close()
        return relation_id

    def insert_many(self, relations: list[Relation]) -> int:
        """
        Insère plusieurs relations en batch.

        Args:
            relations: Liste de Relation à insérer

        Returns:
            Nombre de relations insérées
        """
        if not relations:
            return 0

        data = relations[0].to_dict(exclude_id=True)
        columns = list(data.keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        params = [tuple(r.to_dict(exclude_id=True).values()) for r in relations]
        return self.db.execute_many(sql, params)

    def insert_or_increment(self, relation: Relation) -> int:
        """
        Insère une relation ou incrémente le count si elle existe.

        Args:
            relation: Instance de Relation

        Returns:
            ID de la relation
        """
        existing = self.db.fetch_one(
            f"""
            SELECT id, count FROM {self.TABLE}
            WHERE source_id = ? AND target_id = ? AND relation_type = ?
            """,
            (relation.source_id, relation.target_id, relation.relation_type),
        )

        if existing:
            self.db.execute(
                f"UPDATE {self.TABLE} SET count = count + 1 WHERE id = ?",
                (existing["id"],),
            )
            return existing["id"]
        else:
            return self.insert(relation)

    def get_by_id(self, relation_id: int) -> Optional[Relation]:
        """
        Récupère une relation par son ID.

        Args:
            relation_id: ID de la relation

        Returns:
            Instance de Relation ou None
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE id = ?",
            (relation_id,),
        )
        if row:
            return Relation.from_row(row)
        return None

    def get_from_symbol(
        self,
        source_id: int,
        relation_type: Optional[str] = None,
    ) -> list[Relation]:
        """
        Récupère les relations depuis un symbole (source).

        Args:
            source_id: ID du symbole source
            relation_type: Type de relation (optionnel)

        Returns:
            Liste de Relation depuis ce symbole
        """
        if relation_type:
            rows = self.db.fetch_all(
                f"SELECT * FROM {self.TABLE} WHERE source_id = ? AND relation_type = ?",
                (source_id, relation_type),
            )
        else:
            rows = self.db.fetch_all(
                f"SELECT * FROM {self.TABLE} WHERE source_id = ?",
                (source_id,),
            )
        return [Relation.from_row(row) for row in rows]

    def get_to_symbol(
        self,
        target_id: int,
        relation_type: Optional[str] = None,
    ) -> list[Relation]:
        """
        Récupère les relations vers un symbole (target).

        Args:
            target_id: ID du symbole cible
            relation_type: Type de relation (optionnel)

        Returns:
            Liste de Relation vers ce symbole
        """
        if relation_type:
            rows = self.db.fetch_all(
                f"SELECT * FROM {self.TABLE} WHERE target_id = ? AND relation_type = ?",
                (target_id, relation_type),
            )
        else:
            rows = self.db.fetch_all(
                f"SELECT * FROM {self.TABLE} WHERE target_id = ?",
                (target_id,),
            )
        return [Relation.from_row(row) for row in rows]

    def get_callers(self, target_id: int) -> list[Relation]:
        """
        Récupère tous les appels vers un symbole.

        Args:
            target_id: ID du symbole cible

        Returns:
            Liste de Relation de type 'calls'
        """
        return self.get_to_symbol(target_id, "calls")

    def get_callees(self, source_id: int) -> list[Relation]:
        """
        Récupère tous les appels depuis un symbole.

        Args:
            source_id: ID du symbole source

        Returns:
            Liste de Relation de type 'calls'
        """
        return self.get_from_symbol(source_id, "calls")

    def delete(self, relation_id: int) -> bool:
        """
        Supprime une relation.

        Args:
            relation_id: ID de la relation

        Returns:
            True si supprimée
        """
        cursor = self.db.execute(
            f"DELETE FROM {self.TABLE} WHERE id = ?",
            (relation_id,),
        )
        deleted = cursor.rowcount > 0
        cursor.close()
        return deleted

    def delete_by_symbol(self, symbol_id: int) -> int:
        """
        Supprime toutes les relations impliquant un symbole.

        Args:
            symbol_id: ID du symbole

        Returns:
            Nombre de relations supprimées
        """
        cursor = self.db.execute(
            f"DELETE FROM {self.TABLE} WHERE source_id = ? OR target_id = ?",
            (symbol_id, symbol_id),
        )
        count = cursor.rowcount
        cursor.close()
        return count

    def delete_by_source(self, source_id: int) -> int:
        """
        Supprime toutes les relations depuis un symbole source.

        Args:
            source_id: ID du symbole source

        Returns:
            Nombre de relations supprimées
        """
        cursor = self.db.execute(
            f"DELETE FROM {self.TABLE} WHERE source_id = ?",
            (source_id,),
        )
        count = cursor.rowcount
        cursor.close()
        return count

    def count(self) -> int:
        """Compte le nombre total de relations."""
        return self.db.fetch_scalar(f"SELECT COUNT(*) FROM {self.TABLE}") or 0

    def count_by_type(self) -> dict[str, int]:
        """Compte les relations par type."""
        rows = self.db.fetch_all(
            f"""
            SELECT relation_type, COUNT(*) as count
            FROM {self.TABLE}
            GROUP BY relation_type
            ORDER BY count DESC
            """
        )
        return {row["relation_type"]: row["count"] for row in rows}


# =============================================================================
# FILE RELATION REPOSITORY
# =============================================================================

class FileRelationRepository(BaseRepository):
    """
    Repository pour la table `file_relations`.

    Gère les relations de haut niveau entre fichiers.
    """

    TABLE = "file_relations"

    def insert(self, relation: FileRelation) -> int:
        """
        Insère une nouvelle relation entre fichiers.

        Args:
            relation: Instance de FileRelation

        Returns:
            ID de la relation créée
        """
        data = relation.to_dict(exclude_id=True)
        columns = list(data.keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        cursor = self.db.execute(sql, tuple(data.values()))
        rel_id = cursor.lastrowid
        cursor.close()
        return rel_id

    def insert_or_ignore(self, relation: FileRelation) -> Optional[int]:
        """
        Insère une relation ou ignore si elle existe déjà.

        Args:
            relation: Instance de FileRelation

        Returns:
            ID de la relation ou None si déjà existante
        """
        data = relation.to_dict(exclude_id=True)
        columns = list(data.keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT OR IGNORE INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        cursor = self.db.execute(sql, tuple(data.values()))
        rel_id = cursor.lastrowid if cursor.rowcount > 0 else None
        cursor.close()
        return rel_id

    def get_includes(self, source_file_id: int) -> list[FileRelation]:
        """
        Récupère les fichiers inclus par un fichier.

        Args:
            source_file_id: ID du fichier source

        Returns:
            Liste de FileRelation
        """
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE source_file_id = ? AND relation_type = 'includes'
            """,
            (source_file_id,),
        )
        return [FileRelation.from_row(row) for row in rows]

    def get_included_by(self, target_file_id: int) -> list[FileRelation]:
        """
        Récupère les fichiers qui incluent un fichier.

        Args:
            target_file_id: ID du fichier cible

        Returns:
            Liste de FileRelation
        """
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE target_file_id = ? AND relation_type = 'includes'
            """,
            (target_file_id,),
        )
        return [FileRelation.from_row(row) for row in rows]

    def delete_by_source(self, source_file_id: int) -> int:
        """
        Supprime les relations depuis un fichier.

        Args:
            source_file_id: ID du fichier source

        Returns:
            Nombre de relations supprimées
        """
        cursor = self.db.execute(
            f"DELETE FROM {self.TABLE} WHERE source_file_id = ?",
            (source_file_id,),
        )
        count = cursor.rowcount
        cursor.close()
        return count


# =============================================================================
# ERROR HISTORY REPOSITORY
# =============================================================================

class ErrorHistoryRepository(BaseRepository):
    """
    Repository pour la table `error_history`.

    Gère l'historique des erreurs et bugs.
    """

    TABLE = "error_history"

    def insert(self, error: ErrorHistory) -> int:
        """
        Insère une nouvelle erreur dans l'historique.

        Args:
            error: Instance de ErrorHistory

        Returns:
            ID de l'erreur créée
        """
        data = error.to_dict(exclude_id=True)
        columns = list(data.keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        cursor = self.db.execute(sql, tuple(data.values()))
        error_id = cursor.lastrowid
        cursor.close()

        logger.info(f"Recorded error: {error.title} (id={error_id})")
        return error_id

    def get_by_id(self, error_id: int) -> Optional[ErrorHistory]:
        """
        Récupère une erreur par son ID.

        Args:
            error_id: ID de l'erreur

        Returns:
            Instance de ErrorHistory ou None
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE id = ?",
            (error_id,),
        )
        if row:
            return ErrorHistory.from_row(row)
        return None

    def get_by_file(
        self,
        file_path: str,
        limit: int = 10,
        severity: Optional[str] = None,
    ) -> list[ErrorHistory]:
        """
        Récupère les erreurs pour un fichier.

        Args:
            file_path: Chemin du fichier
            limit: Nombre maximum de résultats
            severity: Filtrer par sévérité minimum (optionnel)

        Returns:
            Liste de ErrorHistory
        """
        sql = f"SELECT * FROM {self.TABLE} WHERE file_path = ?"
        params: list[Any] = [file_path]

        if severity:
            severity_order = ["critical", "high", "medium", "low"]
            if severity in severity_order:
                severity_levels = severity_order[:severity_order.index(severity) + 1]
                placeholders = ", ".join(["?"] * len(severity_levels))
                sql += f" AND severity IN ({placeholders})"
                params.extend(severity_levels)

        sql += " ORDER BY discovered_at DESC LIMIT ?"
        params.append(limit)

        rows = self.db.fetch_all(sql, tuple(params))
        return [ErrorHistory.from_row(row) for row in rows]

    def get_by_symbol(
        self,
        symbol_name: str,
        file_path: Optional[str] = None,
        limit: int = 10,
    ) -> list[ErrorHistory]:
        """
        Récupère les erreurs pour un symbole.

        Args:
            symbol_name: Nom du symbole
            file_path: Chemin du fichier (optionnel)
            limit: Nombre maximum de résultats

        Returns:
            Liste de ErrorHistory
        """
        if file_path:
            rows = self.db.fetch_all(
                f"""
                SELECT * FROM {self.TABLE}
                WHERE symbol_name = ? AND file_path = ?
                ORDER BY discovered_at DESC LIMIT ?
                """,
                (symbol_name, file_path, limit),
            )
        else:
            rows = self.db.fetch_all(
                f"""
                SELECT * FROM {self.TABLE}
                WHERE symbol_name = ?
                ORDER BY discovered_at DESC LIMIT ?
                """,
                (symbol_name, limit),
            )
        return [ErrorHistory.from_row(row) for row in rows]

    def get_by_module(
        self,
        module: str,
        limit: int = 20,
    ) -> list[ErrorHistory]:
        """
        Récupère les erreurs pour un module.

        Args:
            module: Nom du module
            limit: Nombre maximum de résultats

        Returns:
            Liste de ErrorHistory
        """
        rows = self.db.fetch_all(
            f"""
            SELECT e.* FROM {self.TABLE} e
            JOIN files f ON e.file_id = f.id
            WHERE f.module = ?
            ORDER BY e.discovered_at DESC LIMIT ?
            """,
            (module, limit),
        )
        return [ErrorHistory.from_row(row) for row in rows]

    def get_recent(
        self,
        days: int = 30,
        limit: int = 50,
        severity: Optional[str] = None,
        error_type: Optional[str] = None,
    ) -> list[ErrorHistory]:
        """
        Récupère les erreurs récentes.

        Args:
            days: Nombre de jours en arrière
            limit: Nombre maximum de résultats
            severity: Filtrer par sévérité
            error_type: Filtrer par type d'erreur

        Returns:
            Liste de ErrorHistory
        """
        cutoff_date = (datetime.now() - timedelta(days=days)).isoformat()

        sql = f"SELECT * FROM {self.TABLE} WHERE discovered_at >= ?"
        params: list[Any] = [cutoff_date]

        if severity:
            sql += " AND severity = ?"
            params.append(severity)

        if error_type:
            sql += " AND error_type = ?"
            params.append(error_type)

        sql += " ORDER BY discovered_at DESC LIMIT ?"
        params.append(limit)

        rows = self.db.fetch_all(sql, tuple(params))
        return [ErrorHistory.from_row(row) for row in rows]

    def get_regressions(self, limit: int = 20) -> list[ErrorHistory]:
        """
        Récupère les erreurs marquées comme régressions.

        Args:
            limit: Nombre maximum de résultats

        Returns:
            Liste de ErrorHistory de type régression
        """
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE is_regression = 1
            ORDER BY discovered_at DESC LIMIT ?
            """,
            (limit,),
        )
        return [ErrorHistory.from_row(row) for row in rows]

    def get_unresolved(self, limit: int = 50) -> list[ErrorHistory]:
        """
        Récupère les erreurs non résolues.

        Args:
            limit: Nombre maximum de résultats

        Returns:
            Liste de ErrorHistory sans resolved_at
        """
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE resolved_at IS NULL
            ORDER BY
                CASE severity
                    WHEN 'critical' THEN 1
                    WHEN 'high' THEN 2
                    WHEN 'medium' THEN 3
                    WHEN 'low' THEN 4
                    ELSE 5
                END,
                discovered_at DESC
            LIMIT ?
            """,
            (limit,),
        )
        return [ErrorHistory.from_row(row) for row in rows]

    def get_by_type(self, error_type: str, limit: int = 50) -> list[ErrorHistory]:
        """
        Récupère les erreurs par type.

        Args:
            error_type: Type d'erreur
            limit: Nombre maximum de résultats

        Returns:
            Liste de ErrorHistory
        """
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE error_type = ?
            ORDER BY discovered_at DESC LIMIT ?
            """,
            (error_type, limit),
        )
        return [ErrorHistory.from_row(row) for row in rows]

    def get_by_severity(self, severity: str, limit: int = 50) -> list[ErrorHistory]:
        """
        Récupère les erreurs par sévérité.

        Args:
            severity: Niveau de sévérité
            limit: Nombre maximum de résultats

        Returns:
            Liste de ErrorHistory
        """
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE severity = ?
            ORDER BY discovered_at DESC LIMIT ?
            """,
            (severity, limit),
        )
        return [ErrorHistory.from_row(row) for row in rows]

    def update(self, error_id: int, **kwargs: Any) -> bool:
        """
        Met à jour une erreur.

        Args:
            error_id: ID de l'erreur
            **kwargs: Colonnes à mettre à jour

        Returns:
            True si mise à jour
        """
        if not kwargs:
            return False

        sql, values = self._build_update_sql(self.TABLE, "id", **kwargs)
        values.append(error_id)

        cursor = self.db.execute(sql, tuple(values))
        updated = cursor.rowcount > 0
        cursor.close()
        return updated

    def mark_resolved(
        self,
        error_id: int,
        resolution: str,
        fix_commit: Optional[str] = None,
    ) -> bool:
        """
        Marque une erreur comme résolue.

        Args:
            error_id: ID de l'erreur
            resolution: Description de la résolution
            fix_commit: Commit de correction

        Returns:
            True si mise à jour
        """
        return self.update(
            error_id,
            resolved_at=datetime.now().isoformat(),
            resolution=resolution,
            fix_commit=fix_commit,
        )

    def count(self) -> int:
        """Compte le nombre total d'erreurs."""
        return self.db.fetch_scalar(f"SELECT COUNT(*) FROM {self.TABLE}") or 0

    def count_by_severity(self) -> dict[str, int]:
        """Compte les erreurs par sévérité."""
        rows = self.db.fetch_all(
            f"""
            SELECT severity, COUNT(*) as count
            FROM {self.TABLE}
            GROUP BY severity
            """
        )
        return {row["severity"]: row["count"] for row in rows}

    def count_by_type(self) -> dict[str, int]:
        """Compte les erreurs par type."""
        rows = self.db.fetch_all(
            f"""
            SELECT error_type, COUNT(*) as count
            FROM {self.TABLE}
            GROUP BY error_type
            ORDER BY count DESC
            """
        )
        return {row["error_type"]: row["count"] for row in rows}


# =============================================================================
# PIPELINE RUN REPOSITORY
# =============================================================================

class PipelineRunRepository(BaseRepository):
    """
    Repository pour la table `pipeline_runs`.

    Gère l'historique des exécutions du pipeline.
    """

    TABLE = "pipeline_runs"

    def insert(self, run: PipelineRun) -> int:
        """
        Insère un nouveau run du pipeline.

        Args:
            run: Instance de PipelineRun

        Returns:
            ID du run créé
        """
        data = run.to_dict(exclude_id=True)
        columns = list(data.keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        cursor = self.db.execute(sql, tuple(data.values()))
        run_id = cursor.lastrowid
        cursor.close()

        logger.info(f"Recorded pipeline run: {run.run_id} (id={run_id})")
        return run_id

    def get_by_id(self, run_db_id: int) -> Optional[PipelineRun]:
        """
        Récupère un run par son ID de base de données.

        Args:
            run_db_id: ID dans la base

        Returns:
            Instance de PipelineRun ou None
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE id = ?",
            (run_db_id,),
        )
        if row:
            return PipelineRun.from_row(row)
        return None

    def get_by_run_id(self, run_id: str) -> Optional[PipelineRun]:
        """
        Récupère un run par son UUID.

        Args:
            run_id: UUID du run

        Returns:
            Instance de PipelineRun ou None
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE run_id = ?",
            (run_id,),
        )
        if row:
            return PipelineRun.from_row(row)
        return None

    def get_by_commit(self, commit_hash: str) -> list[PipelineRun]:
        """
        Récupère les runs pour un commit.

        Args:
            commit_hash: Hash du commit

        Returns:
            Liste de PipelineRun
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} WHERE commit_hash = ? ORDER BY started_at DESC",
            (commit_hash,),
        )
        return [PipelineRun.from_row(row) for row in rows]

    def get_recent(self, limit: int = 20) -> list[PipelineRun]:
        """
        Récupère les runs récents.

        Args:
            limit: Nombre maximum de résultats

        Returns:
            Liste de PipelineRun
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} ORDER BY started_at DESC LIMIT ?",
            (limit,),
        )
        return [PipelineRun.from_row(row) for row in rows]

    def get_by_status(self, status: str, limit: int = 20) -> list[PipelineRun]:
        """
        Récupère les runs par statut.

        Args:
            status: Statut recherché
            limit: Nombre maximum de résultats

        Returns:
            Liste de PipelineRun
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} WHERE status = ? ORDER BY started_at DESC LIMIT ?",
            (status, limit),
        )
        return [PipelineRun.from_row(row) for row in rows]

    def update(self, run_db_id: int, **kwargs: Any) -> bool:
        """
        Met à jour un run.

        Args:
            run_db_id: ID du run dans la base
            **kwargs: Colonnes à mettre à jour

        Returns:
            True si mis à jour
        """
        if not kwargs:
            return False

        sql, values = self._build_update_sql(self.TABLE, "id", **kwargs)
        values.append(run_db_id)

        cursor = self.db.execute(sql, tuple(values))
        updated = cursor.rowcount > 0
        cursor.close()
        return updated

    def complete_run(
        self,
        run_db_id: int,
        status: str,
        overall_score: Optional[int] = None,
        recommendation: Optional[str] = None,
    ) -> bool:
        """
        Marque un run comme terminé.

        Args:
            run_db_id: ID du run
            status: Statut final
            overall_score: Score global
            recommendation: Recommandation

        Returns:
            True si mis à jour
        """
        return self.update(
            run_db_id,
            status=status,
            completed_at=datetime.now().isoformat(),
            overall_score=overall_score,
            recommendation=recommendation,
        )


# =============================================================================
# SNAPSHOT SYMBOL REPOSITORY
# =============================================================================

class SnapshotSymbolRepository(BaseRepository):
    """
    Repository pour la table `snapshot_symbols`.

    Gère les snapshots temporaires de symboles pour comparaison.
    """

    TABLE = "snapshot_symbols"

    def insert(self, snapshot: SnapshotSymbol) -> int:
        """
        Insère un snapshot de symbole.

        Args:
            snapshot: Instance de SnapshotSymbol

        Returns:
            ID du snapshot créé
        """
        data = snapshot.to_dict(exclude_id=True)

        # Set created_at to current timestamp if not provided
        if not data.get("created_at"):
            data["created_at"] = datetime.now().isoformat()

        columns = list(data.keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        cursor = self.db.execute(sql, tuple(data.values()))
        snap_id = cursor.lastrowid
        cursor.close()
        return snap_id

    def insert_many(self, snapshots: list[SnapshotSymbol]) -> int:
        """
        Insère plusieurs snapshots en batch.

        Args:
            snapshots: Liste de SnapshotSymbol

        Returns:
            Nombre de snapshots insérés
        """
        if not snapshots:
            return 0

        now = datetime.now().isoformat()
        params = []
        for s in snapshots:
            data = s.to_dict(exclude_id=True)
            if not data.get("created_at"):
                data["created_at"] = now
            params.append(tuple(data.values()))

        columns = list(snapshots[0].to_dict(exclude_id=True).keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        return self.db.execute_many(sql, params)

    def get_by_run(self, run_id: str) -> list[SnapshotSymbol]:
        """
        Récupère les snapshots d'un run.

        Args:
            run_id: UUID du run

        Returns:
            Liste de SnapshotSymbol
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} WHERE run_id = ?",
            (run_id,),
        )
        return [SnapshotSymbol.from_row(row) for row in rows]

    def delete_by_run(self, run_id: str) -> int:
        """
        Supprime les snapshots d'un run.

        Args:
            run_id: UUID du run

        Returns:
            Nombre de snapshots supprimés
        """
        cursor = self.db.execute(
            f"DELETE FROM {self.TABLE} WHERE run_id = ?",
            (run_id,),
        )
        count = cursor.rowcount
        cursor.close()
        return count

    def cleanup_old(self, days: int = 7) -> int:
        """
        Nettoie les snapshots plus vieux que N jours.

        Args:
            days: Âge maximum en jours

        Returns:
            Nombre de snapshots supprimés
        """
        cutoff = (datetime.now() - timedelta(days=days)).isoformat()
        cursor = self.db.execute(
            f"DELETE FROM {self.TABLE} WHERE created_at < ?",
            (cutoff,),
        )
        count = cursor.rowcount
        cursor.close()

        logger.info(f"Cleaned up {count} old snapshots")
        return count


# =============================================================================
# PATTERN REPOSITORY
# =============================================================================

class PatternRepository(BaseRepository):
    """
    Repository pour la table `patterns`.

    Gère les patterns de code à respecter.
    """

    TABLE = "patterns"

    def insert(self, pattern: Pattern) -> int:
        """
        Insère un nouveau pattern.

        Args:
            pattern: Instance de Pattern

        Returns:
            ID du pattern créé
        """
        data = pattern.to_dict(exclude_id=True)

        # Set created_at to current timestamp if not provided
        if not data.get("created_at"):
            data["created_at"] = datetime.now().isoformat()

        columns = list(data.keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        cursor = self.db.execute(sql, tuple(data.values()))
        pattern_id = cursor.lastrowid
        cursor.close()

        logger.info(f"Inserted pattern: {pattern.name} (id={pattern_id})")
        return pattern_id

    def get_by_id(self, pattern_id: int) -> Optional[Pattern]:
        """
        Récupère un pattern par son ID.

        Args:
            pattern_id: ID du pattern

        Returns:
            Instance de Pattern ou None
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE id = ?",
            (pattern_id,),
        )
        if row:
            return Pattern.from_row(row)
        return None

    def get_by_name(self, name: str) -> Optional[Pattern]:
        """
        Récupère un pattern par son nom.

        Args:
            name: Nom du pattern

        Returns:
            Instance de Pattern ou None
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE name = ?",
            (name,),
        )
        if row:
            return Pattern.from_row(row)
        return None

    def get_all_active(self) -> list[Pattern]:
        """
        Récupère tous les patterns actifs.

        Returns:
            Liste de Pattern actifs
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} WHERE is_active = 1 ORDER BY category, name"
        )
        return [Pattern.from_row(row) for row in rows]

    def get_for_file(self, file_path: str) -> list[Pattern]:
        """
        Récupère les patterns applicables à un fichier.

        Args:
            file_path: Chemin du fichier

        Returns:
            Liste de Pattern applicables
        """
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE is_active = 1
            AND (
                scope = 'project'
                OR file_pattern IS NOT NULL
            )
            ORDER BY category, name
            """
        )

        patterns = []
        for row in rows:
            pattern = Pattern.from_row(row)
            if pattern.file_pattern:
                if fnmatch.fnmatch(file_path, pattern.file_pattern):
                    patterns.append(pattern)
            elif pattern.scope == "project":
                patterns.append(pattern)

        return patterns

    def get_for_module(self, module: str) -> list[Pattern]:
        """
        Récupère les patterns applicables à un module.

        Args:
            module: Nom du module

        Returns:
            Liste de Pattern applicables
        """
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE is_active = 1
            AND (
                scope = 'project'
                OR module = ?
            )
            ORDER BY category, name
            """,
            (module,),
        )
        return [Pattern.from_row(row) for row in rows]

    def get_by_category(self, category: str) -> list[Pattern]:
        """
        Récupère les patterns d'une catégorie.

        Args:
            category: Catégorie de patterns

        Returns:
            Liste de Pattern
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} WHERE category = ? AND is_active = 1 ORDER BY name",
            (category,),
        )
        return [Pattern.from_row(row) for row in rows]

    def update(self, pattern_id: int, **kwargs: Any) -> bool:
        """
        Met à jour un pattern.

        Args:
            pattern_id: ID du pattern
            **kwargs: Colonnes à mettre à jour

        Returns:
            True si mis à jour
        """
        if not kwargs:
            return False

        kwargs["updated_at"] = datetime.now().isoformat()

        sql, values = self._build_update_sql(self.TABLE, "id", **kwargs)
        values.append(pattern_id)

        cursor = self.db.execute(sql, tuple(values))
        updated = cursor.rowcount > 0
        cursor.close()
        return updated

    def deactivate(self, pattern_id: int) -> bool:
        """
        Désactive un pattern.

        Args:
            pattern_id: ID du pattern

        Returns:
            True si désactivé
        """
        return self.update(pattern_id, is_active=0)

    def get_categories(self) -> list[str]:
        """
        Liste toutes les catégories de patterns.

        Returns:
            Liste des catégories
        """
        rows = self.db.fetch_all(
            f"SELECT DISTINCT category FROM {self.TABLE} ORDER BY category"
        )
        return [row["category"] for row in rows]


# =============================================================================
# ARCHITECTURE DECISION REPOSITORY
# =============================================================================

class ArchitectureDecisionRepository(BaseRepository):
    """
    Repository pour la table `architecture_decisions`.

    Gère les décisions architecturales (ADR).
    """

    TABLE = "architecture_decisions"

    def insert(self, decision: ArchitectureDecision) -> int:
        """
        Insère une nouvelle décision architecturale.

        Args:
            decision: Instance de ArchitectureDecision

        Returns:
            ID de la décision créée
        """
        data = decision.to_dict(exclude_id=True)
        columns = list(data.keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        cursor = self.db.execute(sql, tuple(data.values()))
        decision_id = cursor.lastrowid
        cursor.close()

        logger.info(f"Inserted ADR: {decision.decision_id} (id={decision_id})")
        return decision_id

    def get_by_id(self, db_id: int) -> Optional[ArchitectureDecision]:
        """
        Récupère une décision par son ID de base.

        Args:
            db_id: ID dans la base

        Returns:
            Instance de ArchitectureDecision ou None
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE id = ?",
            (db_id,),
        )
        if row:
            return ArchitectureDecision.from_row(row)
        return None

    def get_by_decision_id(self, decision_id: str) -> Optional[ArchitectureDecision]:
        """
        Récupère une décision par son identifiant.

        Args:
            decision_id: Identifiant de la décision (ex: "ADR-001")

        Returns:
            Instance de ArchitectureDecision ou None
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE decision_id = ?",
            (decision_id,),
        )
        if row:
            return ArchitectureDecision.from_row(row)
        return None

    def get_all(self, status: Optional[str] = None) -> list[ArchitectureDecision]:
        """
        Récupère toutes les décisions.

        Args:
            status: Filtrer par statut (optionnel)

        Returns:
            Liste de ArchitectureDecision
        """
        if status:
            rows = self.db.fetch_all(
                f"SELECT * FROM {self.TABLE} WHERE status = ? ORDER BY decision_id",
                (status,),
            )
        else:
            rows = self.db.fetch_all(
                f"SELECT * FROM {self.TABLE} ORDER BY decision_id"
            )
        return [ArchitectureDecision.from_row(row) for row in rows]

    def get_by_status(self, status: str) -> list[ArchitectureDecision]:
        """
        Récupère les ADRs par statut.

        Args:
            status: Statut (accepted, proposed, deprecated)

        Returns:
            Liste d'ArchitectureDecision
        """
        return self.get_all(status=status)

    def get_accepted(self) -> list[ArchitectureDecision]:
        """
        Récupère les décisions acceptées.

        Returns:
            Liste de ArchitectureDecision acceptées
        """
        return self.get_all(status="accepted")

    def get_for_module(self, module: str) -> list[ArchitectureDecision]:
        """
        Récupère les décisions affectant un module.

        Args:
            module: Nom du module

        Returns:
            Liste de ArchitectureDecision
        """
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE affected_modules_json LIKE ?
            AND status = 'accepted'
            ORDER BY decision_id
            """,
            (f'%"{module}"%',),
        )
        return [ArchitectureDecision.from_row(row) for row in rows]

    def get_for_file(self, file_path: str) -> list[ArchitectureDecision]:
        """
        Récupère les décisions affectant un fichier.

        Args:
            file_path: Chemin du fichier

        Returns:
            Liste de ArchitectureDecision
        """
        rows = self.db.fetch_all(
            f"""
            SELECT * FROM {self.TABLE}
            WHERE affected_files_json LIKE ?
            AND status = 'accepted'
            ORDER BY decision_id
            """,
            (f'%"{file_path}"%',),
        )
        return [ArchitectureDecision.from_row(row) for row in rows]

    def update(self, db_id: int, **kwargs: Any) -> bool:
        """
        Met à jour une décision.

        Args:
            db_id: ID dans la base
            **kwargs: Colonnes à mettre à jour

        Returns:
            True si mise à jour
        """
        if not kwargs:
            return False

        sql, values = self._build_update_sql(self.TABLE, "id", **kwargs)
        values.append(db_id)

        cursor = self.db.execute(sql, tuple(values))
        updated = cursor.rowcount > 0
        cursor.close()
        return updated


# =============================================================================
# CRITICAL PATH REPOSITORY
# =============================================================================

class CriticalPathRepository(BaseRepository):
    """
    Repository pour la table `critical_paths`.

    Gère les chemins critiques du projet.
    """

    TABLE = "critical_paths"

    def insert(self, path: CriticalPath) -> int:
        """
        Insère un nouveau chemin critique.

        Args:
            path: Instance de CriticalPath

        Returns:
            ID du chemin créé
        """
        data = path.to_dict(exclude_id=True)

        # Set added_at to current timestamp if not provided
        if not data.get("added_at"):
            data["added_at"] = datetime.now().isoformat()

        columns = list(data.keys())
        placeholders = ", ".join(["?"] * len(columns))
        sql = f"INSERT INTO {self.TABLE} ({', '.join(columns)}) VALUES ({placeholders})"

        cursor = self.db.execute(sql, tuple(data.values()))
        path_id = cursor.lastrowid
        cursor.close()
        return path_id

    def get_all(self) -> list[CriticalPath]:
        """
        Récupère tous les chemins critiques.

        Returns:
            Liste de CriticalPath
        """
        rows = self.db.fetch_all(
            f"SELECT * FROM {self.TABLE} ORDER BY severity, id"
        )
        return [CriticalPath.from_row(row) for row in rows]

    def get_by_pattern(self, pattern: str) -> Optional[CriticalPath]:
        """
        Récupère un chemin critique par pattern.

        Args:
            pattern: Glob pattern

        Returns:
            CriticalPath ou None
        """
        row = self.db.fetch_one(
            f"SELECT * FROM {self.TABLE} WHERE pattern = ?",
            (pattern,),
        )
        if row:
            return CriticalPath.from_row(row)
        return None

    def is_path_critical(self, file_path: str) -> tuple[bool, Optional[str]]:
        """
        Vérifie si un fichier est dans un chemin critique.

        Args:
            file_path: Chemin du fichier

        Returns:
            Tuple (is_critical, reason): reason est None si pas critique
        """
        paths = self.get_all()
        for cp in paths:
            if fnmatch.fnmatch(file_path, cp.path_pattern):
                return True, cp.reason
        return False, None

    def delete(self, path_id: int) -> bool:
        """
        Supprime un chemin critique.

        Args:
            path_id: ID du chemin

        Returns:
            True si supprimé
        """
        cursor = self.db.execute(
            f"DELETE FROM {self.TABLE} WHERE id = ?",
            (path_id,),
        )
        deleted = cursor.rowcount > 0
        cursor.close()
        return deleted
