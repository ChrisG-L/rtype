"""
Stats cog - /stats, /kills commands.
"""

import discord
from discord import app_commands
from discord.ext import commands

from database.player_stats_repo import PlayerStatsRepository
from utils.embeds import create_stats_embed, create_kills_embed


class StatsCog(commands.Cog):
    """Player stats commands."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot

    async def player_autocomplete(
        self, interaction: discord.Interaction, current: str
    ) -> list[app_commands.Choice[str]]:
        """Autocomplete for player names."""
        names = await PlayerStatsRepository.search_players(current, limit=25)
        return [app_commands.Choice(name=n, value=n) for n in names]

    @app_commands.command(name="stats", description="Affiche les stats d'un joueur")
    @app_commands.describe(player="Nom du joueur")
    @app_commands.autocomplete(player=player_autocomplete)
    async def stats(self, interaction: discord.Interaction, player: str):
        """Display player statistics."""
        await interaction.response.defer()

        stats = await PlayerStatsRepository.get_by_name(player)

        if not stats:
            await interaction.followup.send(
                f"❌ Joueur **{player}** non trouve.", ephemeral=True
            )
            return

        embed = create_stats_embed(stats, player)
        await interaction.followup.send(embed=embed)

    @app_commands.command(
        name="kills", description="Affiche les kills par arme d'un joueur"
    )
    @app_commands.describe(player="Nom du joueur")
    @app_commands.autocomplete(player=player_autocomplete)
    async def kills(self, interaction: discord.Interaction, player: str):
        """Display player kills by weapon."""
        await interaction.response.defer()

        stats = await PlayerStatsRepository.get_by_name(player)

        if not stats:
            await interaction.followup.send(
                f"❌ Joueur **{player}** non trouve.", ephemeral=True
            )
            return

        embed = create_kills_embed(stats, player)
        await interaction.followup.send(embed=embed)


async def setup(bot: commands.Bot):
    """Setup function for the cog."""
    await bot.add_cog(StatsCog(bot))
