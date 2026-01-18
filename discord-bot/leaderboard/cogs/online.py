"""
Online cog - /online command.
Shows currently connected players.

NOTE: This requires the R-Type server to persist session data
to MongoDB collection 'current_sessions'.
"""

import discord
from discord import app_commands
from discord.ext import commands

from database.session_repo import SessionRepository
from utils.embeds import create_online_embed


class OnlineCog(commands.Cog):
    """Online players command."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot

    @app_commands.command(
        name="online", description="Affiche les joueurs actuellement connectes"
    )
    async def online(self, interaction: discord.Interaction):
        """Display online players."""
        await interaction.response.defer()

        sessions = await SessionRepository.get_online_players()

        embed = create_online_embed(sessions)
        await interaction.followup.send(embed=embed)


async def setup(bot: commands.Bot):
    """Setup function for the cog."""
    await bot.add_cog(OnlineCog(bot))
