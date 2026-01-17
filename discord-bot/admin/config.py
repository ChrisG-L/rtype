"""
Configuration for R-Type Admin Discord Bot
"""

import os
from dataclasses import dataclass
from dotenv import load_dotenv

load_dotenv()


@dataclass
class Config:
    """Bot configuration loaded from environment variables."""

    # Discord
    discord_token: str = os.getenv("DISCORD_TOKEN", "")
    admin_channel_id: int = int(os.getenv("ADMIN_CHANNEL_ID", "0"))
    admin_role_id: int = int(os.getenv("ADMIN_ROLE_ID", "0"))

    # R-Type TCP Admin Server connection
    server_host: str = os.getenv("RTYPE_SERVER_HOST", "localhost")
    server_admin_port: int = int(os.getenv("RTYPE_ADMIN_PORT", "4127"))
    admin_token: str = os.getenv("ADMIN_TOKEN", "")

    # Logging
    log_level: str = os.getenv("LOG_LEVEL", "INFO")

    def validate(self) -> bool:
        """Validate required configuration."""
        if not self.discord_token:
            raise ValueError("DISCORD_TOKEN is required")
        if not self.admin_channel_id:
            raise ValueError("ADMIN_CHANNEL_ID is required")
        if not self.admin_token:
            raise ValueError("ADMIN_TOKEN is required")
        return True


config = Config()
