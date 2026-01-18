"""
Leaderboard repository for MongoDB queries.
Handles all leaderboard-related database operations.
"""

from datetime import datetime, timedelta, timezone
from typing import Optional

from .mongodb import MongoDB


class LeaderboardRepository:
    """Repository for leaderboard queries."""

    @staticmethod
    def _get_period_filter(period: str) -> dict:
        """Return timestamp filter for the given period."""
        now = datetime.now(timezone.utc)
        if period == "daily":
            start = now - timedelta(days=1)
            return {"timestamp": {"$gte": int(start.timestamp())}}
        elif period == "weekly":
            # Start of the week (Monday)
            start = now - timedelta(days=now.weekday())
            start = start.replace(hour=0, minute=0, second=0, microsecond=0)
            return {"timestamp": {"$gte": int(start.timestamp())}}
        elif period == "monthly":
            start = now.replace(day=1, hour=0, minute=0, second=0, microsecond=0)
            return {"timestamp": {"$gte": int(start.timestamp())}}
        return {}  # All-time

    @staticmethod
    async def get_top_scores(
        period: str = "all", limit: int = 10, player_count: int = 0
    ) -> list[dict]:
        """Get top N players by best score, optionally filtered by player count.

        Args:
            period: Time period filter (all, daily, weekly, monthly)
            limit: Max number of results
            player_count: Filter by game mode (0=All, 1=Solo, 2=Duo, 3=Trio, 4+=4P+)
        """
        db = MongoDB.get()

        # Build match filter
        match_filter = LeaderboardRepository._get_period_filter(period)
        if player_count > 0:
            match_filter["playerCount"] = player_count

        pipeline = [
            {"$match": match_filter},
            {"$sort": {"score": -1}},
            {
                "$group": {
                    "_id": "$email",
                    "playerName": {"$first": "$playerName"},
                    "score": {"$max": "$score"},
                    "wave": {"$first": "$wave"},
                    "kills": {"$first": "$kills"},
                    "duration": {"$first": "$duration"},
                    "timestamp": {"$first": "$timestamp"},
                    "playerCount": {"$first": "$playerCount"},
                }
            },
            {"$sort": {"score": -1}},
            {"$limit": limit},
        ]

        cursor = db.leaderboard.aggregate(pipeline)
        return [doc async for doc in cursor]

    @staticmethod
    async def get_top_kills(limit: int = 10) -> list[dict]:
        """Get top N players by total kills (from player_stats)."""
        db = MongoDB.get()
        cursor = db.player_stats.find().sort("totalKills", -1).limit(limit)
        return [doc async for doc in cursor]

    @staticmethod
    async def get_top_waves(limit: int = 10) -> list[dict]:
        """Get top N players by best wave reached."""
        db = MongoDB.get()
        cursor = db.player_stats.find().sort("bestWave", -1).limit(limit)
        return [doc async for doc in cursor]

    @staticmethod
    async def get_top_kd(limit: int = 10) -> list[dict]:
        """Get top N players by K/D ratio."""
        db = MongoDB.get()
        pipeline = [
            {"$match": {"totalDeaths": {"$gt": 0}}},
            {"$addFields": {"kdRatio": {"$divide": ["$totalKills", "$totalDeaths"]}}},
            {"$sort": {"kdRatio": -1}},
            {"$limit": limit},
        ]
        cursor = db.player_stats.aggregate(pipeline)
        return [doc async for doc in cursor]

    @staticmethod
    async def get_top_bosses(limit: int = 10) -> list[dict]:
        """Get top N players by boss kills."""
        db = MongoDB.get()
        cursor = db.player_stats.find().sort("bossKills", -1).limit(limit)
        return [doc async for doc in cursor]

    @staticmethod
    async def get_top_playtime(limit: int = 10) -> list[dict]:
        """Get top N players by total playtime."""
        db = MongoDB.get()
        cursor = db.player_stats.find().sort("totalPlaytime", -1).limit(limit)
        return [doc async for doc in cursor]

    @staticmethod
    async def get_top_weapon(weapon: str, limit: int = 10) -> list[dict]:
        """Get top N players by kills with a specific weapon."""
        db = MongoDB.get()
        field = f"{weapon}Kills"  # standardKills, spreadKills, etc.
        cursor = db.player_stats.find().sort(field, -1).limit(limit)
        return [doc async for doc in cursor]

    @staticmethod
    async def get_player_rank(
        player_name: str, period: str = "all", player_count: int = 0
    ) -> Optional[tuple[int, int]]:
        """
        Get a player's rank and total player count.
        Returns (rank, total_players) or None if not found.

        Args:
            player_name: Name of the player
            period: Time period filter (all, daily, weekly, monthly)
            player_count: Filter by game mode (0=All, 1=Solo, 2=Duo, 3=Trio, 4+=4P+)
        """
        db = MongoDB.get()

        # Build base match filter
        base_filter = LeaderboardRepository._get_period_filter(period)
        if player_count > 0:
            base_filter["playerCount"] = player_count

        # Get the player's best score
        player_filter = {"playerName": player_name, **base_filter}
        pipeline = [
            {"$match": player_filter},
            {"$group": {"_id": "$playerName", "bestScore": {"$max": "$score"}}},
        ]
        cursor = db.leaderboard.aggregate(pipeline)
        player_doc = await cursor.to_list(1)

        if not player_doc:
            return None

        player_best = player_doc[0]["bestScore"]

        # Count players with higher score
        count_pipeline = [
            {"$match": base_filter},
            {"$group": {"_id": "$email", "bestScore": {"$max": "$score"}}},
            {"$match": {"bestScore": {"$gt": player_best}}},
            {"$count": "higher"},
        ]
        cursor = db.leaderboard.aggregate(count_pipeline)
        result = await cursor.to_list(1)
        higher_count = result[0]["higher"] if result else 0

        # Count total players
        total_pipeline = [
            {"$match": base_filter},
            {"$group": {"_id": "$email"}},
            {"$count": "total"},
        ]
        cursor = db.leaderboard.aggregate(total_pipeline)
        total_result = await cursor.to_list(1)
        total_count = total_result[0]["total"] if total_result else 0

        return (higher_count + 1, total_count)

    @staticmethod
    async def get_all_player_names() -> list[str]:
        """Get all unique player names for autocomplete."""
        db = MongoDB.get()
        cursor = db.player_stats.find({}, {"playerName": 1})
        names = []
        async for doc in cursor:
            if "playerName" in doc:
                names.append(doc["playerName"])
        return names
