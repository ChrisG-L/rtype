"""
MongoDB client singleton for async operations.
Uses motor for async MongoDB access.
"""

from motor.motor_asyncio import AsyncIOMotorClient, AsyncIOMotorDatabase
from typing import Optional
import logging

logger = logging.getLogger(__name__)


class MongoDB:
    """Singleton MongoDB client."""

    _instance: Optional["MongoDB"] = None

    def __init__(self, client: AsyncIOMotorClient, db_name: str):
        self.client = client
        self.db: AsyncIOMotorDatabase = client[db_name]

        # Collections (matching R-Type server)
        self.player_stats = self.db["player_stats"]
        self.users = self.db["user"]

    @classmethod
    async def connect(cls, uri: str, db_name: str) -> "MongoDB":
        """Connect to MongoDB and return singleton instance."""
        if cls._instance is None:
            client = AsyncIOMotorClient(uri)
            # Test connection
            await client.admin.command("ping")
            cls._instance = cls(client, db_name)
            logger.info(f"MongoDB connected to {db_name}")
        return cls._instance

    @classmethod
    def get(cls) -> Optional["MongoDB"]:
        """Get the singleton instance. Returns None if not connected."""
        return cls._instance

    @classmethod
    async def close(cls) -> None:
        """Close the MongoDB connection."""
        if cls._instance is not None:
            cls._instance.client.close()
            cls._instance = None
            logger.info("MongoDB disconnected")
