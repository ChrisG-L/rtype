"""
Moderation repository for MongoDB queries.
Handles temp bans and moderation history.
"""

from datetime import datetime, timezone
from typing import Optional
from .mongodb import MongoDB


class ModerationRepository:
    """Repository for moderation-related queries."""

    # Collection name for temp bans
    TEMP_BANS_COLLECTION = "temp_bans"
    # Collection name for moderation history
    MOD_HISTORY_COLLECTION = "moderation_history"

    @staticmethod
    async def add_temp_ban(
        email: str,
        moderator: str,
        reason: str,
        duration_minutes: int,
        expires_at: datetime,
    ) -> bool:
        """Add a temporary ban record.

        Args:
            email: User email
            moderator: Discord username of moderator
            reason: Ban reason
            duration_minutes: Duration in minutes
            expires_at: Datetime when ban expires

        Returns:
            True if successful
        """
        db = MongoDB.get()
        if db is None:
            return False

        doc = {
            "email": email,
            "moderator": moderator,
            "reason": reason,
            "duration_minutes": duration_minutes,
            "banned_at": datetime.now(timezone.utc),
            "expires_at": expires_at,
            "unbanned": False,
        }

        await db.db[ModerationRepository.TEMP_BANS_COLLECTION].insert_one(doc)
        return True

    @staticmethod
    async def get_expired_temp_bans() -> list[dict]:
        """Get all expired temp bans that haven't been unbanned yet.

        Returns:
            List of expired temp ban documents
        """
        db = MongoDB.get()
        if db is None:
            return []

        now = datetime.now(timezone.utc)
        cursor = db.db[ModerationRepository.TEMP_BANS_COLLECTION].find({
            "expires_at": {"$lte": now},
            "unbanned": False,
        })
        return [doc async for doc in cursor]

    @staticmethod
    async def mark_temp_ban_unbanned(email: str) -> bool:
        """Mark a temp ban as unbanned.

        Args:
            email: User email

        Returns:
            True if updated
        """
        db = MongoDB.get()
        if db is None:
            return False

        result = await db.db[ModerationRepository.TEMP_BANS_COLLECTION].update_many(
            {"email": email, "unbanned": False},
            {"$set": {"unbanned": True, "unbanned_at": datetime.now(timezone.utc)}},
        )
        return result.modified_count > 0

    @staticmethod
    async def get_active_temp_ban(email: str) -> Optional[dict]:
        """Get active temp ban for a user.

        Args:
            email: User email

        Returns:
            Active temp ban document or None
        """
        db = MongoDB.get()
        if db is None:
            return None

        now = datetime.now(timezone.utc)
        return await db.db[ModerationRepository.TEMP_BANS_COLLECTION].find_one({
            "email": email,
            "expires_at": {"$gt": now},
            "unbanned": False,
        })

    @staticmethod
    async def add_mod_action(
        action: str,
        target_email: str,
        moderator: str,
        reason: str = "",
        details: Optional[dict] = None,
    ) -> bool:
        """Log a moderation action.

        Args:
            action: Action type (ban, unban, kick, tempban)
            target_email: Target user email
            moderator: Discord username of moderator
            reason: Action reason
            details: Additional details (duration, etc.)

        Returns:
            True if successful
        """
        db = MongoDB.get()
        if db is None:
            return False

        doc = {
            "action": action,
            "target_email": target_email,
            "moderator": moderator,
            "reason": reason,
            "details": details or {},
            "timestamp": datetime.now(timezone.utc),
        }

        await db.db[ModerationRepository.MOD_HISTORY_COLLECTION].insert_one(doc)
        return True

    @staticmethod
    async def get_mod_history(
        target_email: Optional[str] = None,
        moderator: Optional[str] = None,
        action: Optional[str] = None,
        limit: int = 50,
    ) -> list[dict]:
        """Get moderation history with optional filters.

        Args:
            target_email: Filter by target user
            moderator: Filter by moderator
            action: Filter by action type
            limit: Max number of results

        Returns:
            List of moderation history documents
        """
        db = MongoDB.get()
        if db is None:
            return []

        query = {}
        if target_email:
            query["target_email"] = target_email
        if moderator:
            query["moderator"] = moderator
        if action:
            query["action"] = action

        cursor = db.db[ModerationRepository.MOD_HISTORY_COLLECTION].find(query).sort(
            "timestamp", -1
        ).limit(limit)

        return [doc async for doc in cursor]

    @staticmethod
    async def get_user_mod_summary(email: str) -> dict:
        """Get moderation summary for a user.

        Args:
            email: User email

        Returns:
            Summary dict with counts per action type
        """
        db = MongoDB.get()
        if db is None:
            return {}

        pipeline = [
            {"$match": {"target_email": email}},
            {"$group": {"_id": "$action", "count": {"$sum": 1}}},
        ]

        cursor = db.db[ModerationRepository.MOD_HISTORY_COLLECTION].aggregate(pipeline)
        summary = {}
        async for doc in cursor:
            summary[doc["_id"]] = doc["count"]

        return summary
