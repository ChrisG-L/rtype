"""
Achievements cog - /achievements command.
"""

import discord
from discord import app_commands
from discord.ext import commands

from database.player_stats_repo import PlayerStatsRepository
from utils.embeds import create_achievements_embed


class AchievementsCog(commands.Cog):
    """Achievement commands."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot

    async def player_autocomplete(
        self, interaction: discord.Interaction, current: str
    ) -> list[app_commands.Choice[str]]:
        """Autocomplete for player names."""
        names = await PlayerStatsRepository.search_players(current, limit=25)
        return [app_commands.Choice(name=n, value=n) for n in names]

    @app_commands.command(
        name="achievements", description="Affiche les succes d'un joueur"
    )
    @app_commands.describe(player="Nom du joueur")
    @app_commands.autocomplete(player=player_autocomplete)
    async def achievements(self, interaction: discord.Interaction, player: str):
        """Display player achievements."""
        await interaction.response.defer()

        # Check if player exists
        stats = await PlayerStatsRepository.get_by_name(player)
        if not stats:
            await interaction.followup.send(
                f"❌ Joueur **{player}** non trouve.", ephemeral=True
            )
            return

        # Get achievements with unlock dates and rarity
        achievements = await PlayerStatsRepository.get_achievements_with_dates(player)
        rarity = await PlayerStatsRepository.get_achievement_rarity()

        embed = create_achievements_embed(achievements, player, rarity)
        await interaction.followup.send(embed=embed)


async def setup(bot: commands.Bot):
    """Setup function for the cog."""
    await bot.add_cog(AchievementsCog(bot))
