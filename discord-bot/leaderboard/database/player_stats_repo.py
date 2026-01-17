"""
Player stats repository for MongoDB queries.
Handles player statistics and achievements.
"""

from typing import Optional

from .mongodb import MongoDB


# Achievement bit positions (matching R-Type server)
ACHIEVEMENTS = {
    "First Blood": 0,  # Get 1 kill
    "Exterminator": 1,  # 1000 total kills
    "Combo Master": 2,  # Achieve 3.0x combo
    "Boss Slayer": 3,  # Kill any boss
    "Survivor": 4,  # Reach wave 20 without dying
    "Speed Demon": 5,  # Wave 10 in under 5 minutes
    "Perfectionist": 6,  # Complete wave without damage
    "Veteran": 7,  # Play 100 games
    "Untouchable": 8,  # Complete game with 0 deaths
    "Weapon Master": 9,  # 100+ kills with each weapon
}


class PlayerStatsRepository:
    """Repository for player statistics queries."""

    @staticmethod
    async def get_by_name(player_name: str) -> Optional[dict]:
        """Get player stats by player name.

        First searches in player_stats, then falls back to user collection.
        """
        db = MongoDB.get()

        # Try player_stats first
        stats = await db.player_stats.find_one({"playerName": player_name})
        if stats:
            return stats

        # Fall back to user collection (player may not have played yet)
        user = await db.users.find_one({"username": player_name})
        if user:
            # Return a minimal stats object from user data
            return {
                "playerName": user.get("username"),
                "email": user.get("email"),
                "gamesPlayed": 0,
                "totalKills": 0,
                "totalDeaths": 0,
                "totalScore": 0,
                "bestScore": 0,
                "bestWave": 0,
                "achievements": 0,
            }

        return None

    @staticmethod
    async def get_by_email(email: str) -> Optional[dict]:
        """Get player stats by email."""
        db = MongoDB.get()
        return await db.player_stats.find_one({"email": email})

    @staticmethod
    async def get_game_history(player_name: str, limit: int = 5) -> list[dict]:
        """Get recent game history for a player."""
        db = MongoDB.get()

        # First get the email from player_stats
        stats = await db.player_stats.find_one({"playerName": player_name})
        if not stats:
            return []

        email = stats.get("email")
        if not email:
            return []

        cursor = (
            db.game_history.find({"email": email})
            .sort("timestamp", -1)
            .limit(limit)
        )
        return [doc async for doc in cursor]

    @staticmethod
    async def get_achievements(player_name: str) -> dict[str, bool]:
        """Get achievements for a player as a dict of achievement_name -> unlocked."""
        db = MongoDB.get()

        stats = await db.player_stats.find_one({"playerName": player_name})
        if not stats:
            return {name: False for name in ACHIEVEMENTS}

        bitfield = stats.get("achievements", 0)

        return {name: bool(bitfield & (1 << bit)) for name, bit in ACHIEVEMENTS.items()}

    @staticmethod
    async def get_weapon_stats(player_name: str) -> dict[str, int]:
        """Get kills per weapon for a player."""
        db = MongoDB.get()

        stats = await db.player_stats.find_one({"playerName": player_name})
        if not stats:
            return {
                "Standard": 0,
                "Spread": 0,
                "Laser": 0,
                "Missile": 0,
                "Wave Cannon": 0,
            }

        return {
            "Standard": stats.get("standardKills", 0),
            "Spread": stats.get("spreadKills", 0),
            "Laser": stats.get("laserKills", 0),
            "Missile": stats.get("missileKills", 0),
            "Wave Cannon": stats.get("waveCannonKills", 0),
        }

    @staticmethod
    async def search_players(query: str, limit: int = 25) -> list[str]:
        """Search player names matching query (for autocomplete).

        Searches in both player_stats (playerName) and user (username) collections.
        If query is empty, returns all players up to limit.
        """
        db = MongoDB.get()
        names = set()

        # Build filter - empty query matches all
        if query:
            player_filter = {"playerName": {"$regex": f"^{query}", "$options": "i"}}
            user_filter = {"username": {"$regex": f"^{query}", "$options": "i"}}
        else:
            player_filter = {}
            user_filter = {}

        # Search in player_stats collection
        cursor = db.player_stats.find(player_filter, {"playerName": 1}).limit(limit)

        async for doc in cursor:
            if "playerName" in doc:
                names.add(doc["playerName"])

        # Also search in user collection (registered users)
        if len(names) < limit:
            remaining = limit - len(names)
            cursor = db.users.find(user_filter, {"username": 1}).limit(remaining)

            async for doc in cursor:
                if "username" in doc:
                    names.add(doc["username"])

        return list(names)[:limit]
