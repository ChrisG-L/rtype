#!/usr/bin/env python3
"""
R-Type Discord Bot - Leaderboard Module
Entry point for the bot.
"""

import asyncio
import logging
import sys

import discord
from discord.ext import commands

from config import Config
from database.mongodb import MongoDB
from cogs import EXTENSIONS


class RTypeBot(commands.Bot):
    """R-Type Leaderboard Discord Bot."""

    def __init__(self):
        intents = discord.Intents.default()
        super().__init__(
            command_prefix="!",
            intents=intents,
            description="R-Type Leaderboard Bot",
        )
        self.db = None

    async def setup_hook(self):
        """Setup hook called before the bot starts."""
        # Connect to MongoDB
        self.db = await MongoDB.connect(Config.MONGODB_URI, Config.MONGODB_DB)

        # Load all cogs
        for extension in EXTENSIONS:
            try:
                await self.load_extension(extension)
                logging.info(f"Loaded extension: {extension}")
            except Exception as e:
                logging.error(f"Failed to load extension {extension}: {e}")

        # Sync slash commands
        if Config.DISCORD_GUILD_ID:
            # Fast sync to specific guild (for development)
            guild = discord.Object(id=Config.DISCORD_GUILD_ID)
            self.tree.copy_global_to(guild=guild)
            await self.tree.sync(guild=guild)
            logging.info(f"Synced commands to guild {Config.DISCORD_GUILD_ID}")
        else:
            # Global sync (takes up to 1 hour to propagate)
            await self.tree.sync()
            logging.info("Synced commands globally")

    async def on_ready(self):
        """Called when bot is ready."""
        logging.info(f"Logged in as {self.user} (ID: {self.user.id})")
        logging.info(f"Connected to {len(self.guilds)} guild(s)")

        # Set activity
        activity = discord.Activity(
            type=discord.ActivityType.watching,
            name="R-Type Leaderboard",
        )
        await self.change_presence(activity=activity)

    async def on_command_error(self, ctx, error):
        """Global error handler."""
        if isinstance(error, commands.CommandNotFound):
            return
        logging.error(f"Command error: {error}")

    async def close(self):
        """Cleanup on shutdown."""
        await MongoDB.close()
        await super().close()


def setup_logging():
    """Configure logging."""
    level = getattr(logging, Config.LOG_LEVEL.upper(), logging.INFO)
    logging.basicConfig(
        level=level,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )
    # Reduce discord.py noise
    logging.getLogger("discord").setLevel(logging.WARNING)
    logging.getLogger("discord.http").setLevel(logging.WARNING)


def main():
    """Main entry point."""
    # Validate config
    try:
        Config.validate()
    except ValueError as e:
        print(f"Configuration error: {e}")
        print("Please check your .env file")
        sys.exit(1)

    setup_logging()

    bot = RTypeBot()

    try:
        bot.run(Config.DISCORD_TOKEN, log_handler=None)
    except discord.LoginFailure:
        logging.error("Invalid Discord token")
        sys.exit(1)
    except KeyboardInterrupt:
        logging.info("Shutting down...")


if __name__ == "__main__":
    main()
