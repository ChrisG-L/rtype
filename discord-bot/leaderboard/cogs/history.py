"""
History cog - /history command.
Shows recent game history for a player.
"""

import discord
from discord import app_commands
from discord.ext import commands

from database.player_stats_repo import PlayerStatsRepository
from utils.embeds import create_history_embed


class HistoryCog(commands.Cog):
    """Game history command."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot

    async def player_autocomplete(
        self, interaction: discord.Interaction, current: str
    ) -> list[app_commands.Choice[str]]:
        """Autocomplete for player names."""
        names = await PlayerStatsRepository.search_players(current, limit=25)
        return [app_commands.Choice(name=n, value=n) for n in names]

    @app_commands.command(
        name="history", description="Affiche les dernieres parties d'un joueur"
    )
    @app_commands.describe(player="Nom du joueur", limit="Nombre de parties (max 10)")
    @app_commands.autocomplete(player=player_autocomplete)
    async def history(
        self, interaction: discord.Interaction, player: str, limit: int = 5
    ):
        """Display player game history."""
        await interaction.response.defer()

        # Clamp limit
        limit = max(1, min(limit, 10))

        history = await PlayerStatsRepository.get_game_history(player, limit)

        if not history:
            # Check if player exists
            stats = await PlayerStatsRepository.get_by_name(player)
            if not stats:
                await interaction.followup.send(
                    f"❌ Joueur **{player}** non trouve.", ephemeral=True
                )
                return

            await interaction.followup.send(
                f"📜 Aucune partie enregistree pour **{player}**.", ephemeral=True
            )
            return

        embed = create_history_embed(history, player)
        await interaction.followup.send(embed=embed)


async def setup(bot: commands.Bot):
    """Setup function for the cog."""
    await bot.add_cog(HistoryCog(bot))
