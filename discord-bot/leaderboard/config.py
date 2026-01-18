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
    # Allowed channels (comma-separated IDs) - if empty, all channels are allowed
    ALLOWED_CHANNEL_IDS: list[int] = [
        int(x.strip()) for x in os.getenv("ALLOWED_CHANNEL_IDS", "").split(",")
        if x.strip().isdigit()
    ]

    # MongoDB
    MONGODB_URI: str = os.getenv("MONGODB_URI", "mongodb://localhost:27017")
    MONGODB_DB: str = os.getenv("MONGODB_DB", "rtype")

    # Auto-announce records channel (optional)
    RECORDS_CHANNEL_ID: int | None = (
        int(os.getenv("RECORDS_CHANNEL_ID")) if os.getenv("RECORDS_CHANNEL_ID") else None
    )
    # Interval in seconds for polling new records (default: 30s)
    RECORDS_POLL_INTERVAL: int = int(os.getenv("RECORDS_POLL_INTERVAL", "30"))

    # Optional
    LOG_LEVEL: str = os.getenv("LOG_LEVEL", "INFO")
    EMBED_COLOR: int = int(os.getenv("EMBED_COLOR", "0x00FF00"), 16)

    @classmethod
    def validate(cls) -> None:
        """Validate required configuration."""
        if not cls.DISCORD_TOKEN:
            raise ValueError("DISCORD_TOKEN is required")
