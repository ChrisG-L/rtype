"""
Session repository for MongoDB queries.
Handles online players and active game sessions.
"""

from .mongodb import MongoDB


class SessionRepository:
    """Repository for active session queries."""

    @staticmethod
    async def get_online_players() -> list[dict]:
        """Get all currently online players."""
        db = MongoDB.get()

        # This collection needs to be populated by the R-Type server
        # when players join/leave games
        cursor = db.current_sessions.find()
        return [doc async for doc in cursor]

    @staticmethod
    async def get_active_rooms() -> list[dict]:
        """Get all active rooms with their players."""
        db = MongoDB.get()

        pipeline = [
            {
                "$group": {
                    "_id": "$roomCode",
                    "players": {
                        "$push": {
                            "playerName": "$playerName",
                            "wave": "$currentWave",
                        }
                    },
                    "playerCount": {"$sum": 1},
                }
            }
        ]

        cursor = db.current_sessions.aggregate(pipeline)
        return [doc async for doc in cursor]

    @staticmethod
    async def get_online_count() -> int:
        """Get count of online players."""
        db = MongoDB.get()
        return await db.current_sessions.count_documents({})
