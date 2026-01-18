"""
Player stats repository for MongoDB queries.
Handles player statistics and achievements.
"""

import re
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
        # Defense in depth: validate input type
        if not isinstance(player_name, str) or len(player_name) > 100:
            return None

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
    async def get_achievements_with_dates(player_name: str) -> dict[str, int]:
        """Get achievements with unlock timestamps.

        Returns dict of achievement_name -> unlockedAt timestamp (0 if not unlocked).
        """
        db = MongoDB.get()

        # Get email from player_stats
        stats = await db.player_stats.find_one({"playerName": player_name})
        if not stats:
            return {name: 0 for name in ACHIEVEMENTS}

        email = stats.get("email")
        if not email:
            return {name: 0 for name in ACHIEVEMENTS}

        # Query achievements collection
        result = {name: 0 for name in ACHIEVEMENTS}
        cursor = db.achievements.find({"email": email})

        # Map type (int) back to achievement name
        type_to_name = {bit: name for name, bit in ACHIEVEMENTS.items()}

        async for doc in cursor:
            ach_type = doc.get("type")
            unlocked_at = doc.get("unlockedAt") or 0
            if ach_type is not None and ach_type in type_to_name:
                result[type_to_name[ach_type]] = unlocked_at

        return result

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
            # Escape regex special characters to prevent injection
            safe_query = re.escape(query)
            player_filter = {"playerName": {"$regex": f"^{safe_query}", "$options": "i"}}
            user_filter = {"username": {"$regex": f"^{safe_query}", "$options": "i"}}
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

    @staticmethod
    async def get_achievement_rarity() -> dict[str, float]:
        """Get the rarity (% of players who have it) for each achievement.

        Returns:
            Dict of achievement_name -> percentage (0-100)
        """
        db = MongoDB.get()

        # Count total players with stats
        total_players = await db.player_stats.count_documents({})
        if total_players == 0:
            return {name: 0.0 for name in ACHIEVEMENTS}

        # Count players with each achievement using bitwise check
        rarity = {}
        for name, bit in ACHIEVEMENTS.items():
            # Count players where the achievement bit is set
            # MongoDB $bitsAllSet checks if specific bits are set
            count = await db.player_stats.count_documents({
                "achievements": {"$bitsAllSet": [bit]}
            })
            rarity[name] = (count / total_players) * 100

        return rarity

    @staticmethod
    async def get_server_stats() -> dict:
        """Get server-wide aggregated statistics."""
        db = MongoDB.get()

        # Count total players
        total_players = await db.player_stats.count_documents({})

        # Aggregate stats from player_stats
        pipeline = [
            {
                "$group": {
                    "_id": None,
                    "totalGames": {"$sum": "$gamesPlayed"},
                    "totalKills": {"$sum": "$totalKills"},
                    "totalDeaths": {"$sum": "$totalDeaths"},
                    "totalPlaytime": {"$sum": "$totalPlaytime"},
                    "totalBossKills": {"$sum": "$bossKills"},
                    "highestScore": {"$max": "$bestScore"},
                    "highestWave": {"$max": "$bestWave"},
                    "highestCombo": {"$max": "$bestCombo"},
                }
            }
        ]

        cursor = db.player_stats.aggregate(pipeline)
        result = await cursor.to_list(1)

        if not result:
            return {"totalPlayers": total_players}

        stats = result[0]
        stats["totalPlayers"] = total_players

        # Find record holders
        score_holder = await db.player_stats.find_one(
            {"bestScore": stats.get("highestScore")}, {"playerName": 1}
        )
        if score_holder:
            stats["scoreHolder"] = score_holder.get("playerName")

        wave_holder = await db.player_stats.find_one(
            {"bestWave": stats.get("highestWave")}, {"playerName": 1}
        )
        if wave_holder:
            stats["waveHolder"] = wave_holder.get("playerName")

        return stats
