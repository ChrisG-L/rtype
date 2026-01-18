#!/usr/bin/env python3
"""
R-Type Admin Discord Bot

Provides admin commands for managing the R-Type game server via Discord.
Connects directly to the server's TCP Admin interface (port 4127).
"""

import asyncio
import logging
import sys
from pathlib import Path

import discord
from discord.ext import commands

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent))

from config import config
from tcp_client import TCPAdminClient
from cogs import setup_cogs
from database import MongoDB

# Configure logging
logging.basicConfig(
    level=getattr(logging, config.log_level.upper(), logging.INFO),
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    handlers=[
        logging.StreamHandler(),
    ]
)
logger = logging.getLogger("rtype-admin")


class RTypeAdminBot(commands.Bot):
    """R-Type Admin Discord Bot."""

    def __init__(self):
        intents = discord.Intents.default()
        intents.message_content = True
        intents.guilds = True

        super().__init__(
            command_prefix="!",  # Fallback, we use slash commands
            intents=intents,
            help_command=None  # Using custom help
        )

        # TCP Client for server communication
        self.tcp_client = TCPAdminClient(
            host=config.server_host,
            port=config.server_admin_port,
            token=config.admin_token
        )

    async def setup_hook(self) -> None:
        """Called when the bot is starting up."""
        logger.info("Setting up bot...")

        # Connect to R-Type server
        try:
            connected = await self.tcp_client.connect()
            if connected:
                logger.info(f"Connected to R-Type server at {config.server_host}:{config.server_admin_port}")
            else:
                logger.warning("Could not connect to R-Type server - commands will retry on use")
        except Exception as e:
            logger.warning(f"Initial connection to R-Type server failed: {e}")

        # Connect to MongoDB (for extended stats)
        try:
            await MongoDB.connect(config.mongodb_uri, config.mongodb_db)
            logger.info(f"Connected to MongoDB ({config.mongodb_db})")
        except Exception as e:
            logger.warning(f"MongoDB connection failed (extended stats unavailable): {e}")

        # Load cogs
        await setup_cogs(self)
        logger.info("Cogs loaded")

        # Sync slash commands
        logger.info("Syncing slash commands...")
        await self.tree.sync()
        logger.info("Slash commands synced")

    async def on_ready(self) -> None:
        """Called when the bot is fully ready."""
        logger.info(f"Logged in as {self.user} (ID: {self.user.id})")
        logger.info(f"Connected to {len(self.guilds)} guild(s)")

        # Set presence
        activity = discord.Activity(
            type=discord.ActivityType.watching,
            name="R-Type Server"
        )
        await self.change_presence(activity=activity)

        if config.admin_channel_id:
            logger.info(f"Admin channel: {config.admin_channel_id}")

    async def on_command_error(self, ctx: commands.Context, error: Exception) -> None:
        """Handle command errors."""
        if isinstance(error, commands.CommandNotFound):
            return
        logger.error(f"Command error: {error}")

    async def close(self) -> None:
        """Clean up when bot is closing."""
        logger.info("Shutting down...")
        await self.tcp_client.disconnect()
        await MongoDB.close()
        await super().close()


async def main():
    """Main entry point."""
    # Validate config
    try:
        config.validate()
    except ValueError as e:
        logger.error(f"Configuration error: {e}")
        sys.exit(1)

    # Create and run bot
    bot = RTypeAdminBot()

    try:
        await bot.start(config.discord_token)
    except discord.LoginFailure:
        logger.error("Invalid Discord token")
        sys.exit(1)
    except KeyboardInterrupt:
        logger.info("Received interrupt signal")
    finally:
        await bot.close()


if __name__ == "__main__":
    asyncio.run(main())
