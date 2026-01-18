"""
Player stats repository for MongoDB queries.
Provides extended stats for /user command.
"""

from typing import Optional
from .mongodb import MongoDB


class PlayerStatsRepository:
    """Repository for player statistics queries."""

    @staticmethod
    async def get_by_email(email: str) -> Optional[dict]:
        """Get player stats by email."""
        db = MongoDB.get()
        if db is None:
            return None
        return await db.player_stats.find_one({"email": email})

    @staticmethod
    async def get_user_by_email(email: str) -> Optional[dict]:
        """Get user document by email."""
        db = MongoDB.get()
        if db is None:
            return None
        return await db.users.find_one({"email": email})
