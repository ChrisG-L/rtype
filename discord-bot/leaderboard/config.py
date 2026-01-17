"""
Configuration module for R-Type Discord Bot.
Loads environment variables from .env file.
"""

import os
from dotenv import load_dotenv

load_dotenv()


class Config:
    """Bot configuration from environment variables."""

    # Discord
    DISCORD_TOKEN: str = os.getenv("DISCORD_TOKEN", "")
    DISCORD_GUILD_ID: int | None = (
        int(os.getenv("DISCORD_GUILD_ID")) if os.getenv("DISCORD_GUILD_ID") else None
    )
    STATS_CHANNEL_ID: int | None = (
        int(os.getenv("STATS_CHANNEL_ID")) if os.getenv("STATS_CHANNEL_ID") else None
    )

    # MongoDB
    MONGODB_URI: str = os.getenv("MONGODB_URI", "mongodb://localhost:27017")
    MONGODB_DB: str = os.getenv("MONGODB_DB", "rtype")

    # Optional
    LOG_LEVEL: str = os.getenv("LOG_LEVEL", "INFO")
    EMBED_COLOR: int = int(os.getenv("EMBED_COLOR", "0x00FF00"), 16)

    @classmethod
    def validate(cls) -> None:
        """Validate required configuration."""
        if not cls.DISCORD_TOKEN:
            raise ValueError("DISCORD_TOKEN is required")
