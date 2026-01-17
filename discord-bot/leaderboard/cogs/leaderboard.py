"""
Leaderboard cog - /leaderboard, /rank, /weapon commands.
"""

import discord
from discord import app_commands
from discord.ext import commands

from database.leaderboard_repo import LeaderboardRepository
from database.player_stats_repo import PlayerStatsRepository
from utils.embeds import create_leaderboard_embed, create_rank_embed


class LeaderboardCog(commands.Cog):
    """Leaderboard commands."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot

    async def player_autocomplete(
        self, interaction: discord.Interaction, current: str
    ) -> list[app_commands.Choice[str]]:
        """Autocomplete for player names."""
        if len(current) < 1:
            return []
        names = await PlayerStatsRepository.search_players(current, limit=25)
        return [app_commands.Choice(name=n, value=n) for n in names]

    @app_commands.command(
        name="leaderboard", description="Affiche le classement des joueurs"
    )
    @app_commands.describe(
        category="Type de classement", period="Periode du classement"
    )
    @app_commands.choices(
        category=[
            app_commands.Choice(name="Score", value="score"),
            app_commands.Choice(name="Kills", value="kills"),
            app_commands.Choice(name="Wave", value="wave"),
            app_commands.Choice(name="K/D Ratio", value="kd"),
            app_commands.Choice(name="Boss Kills", value="bosses"),
            app_commands.Choice(name="Temps de jeu", value="playtime"),
        ]
    )
    @app_commands.choices(
        period=[
            app_commands.Choice(name="All-Time", value="all"),
            app_commands.Choice(name="Weekly", value="weekly"),
            app_commands.Choice(name="Monthly", value="monthly"),
        ]
    )
    async def leaderboard(
        self,
        interaction: discord.Interaction,
        category: str = "score",
        period: str = "all",
    ):
        """Display leaderboard."""
        await interaction.response.defer()

        if category == "score":
            data = await LeaderboardRepository.get_top_scores(period, 10)
        elif category == "kills":
            data = await LeaderboardRepository.get_top_kills(10)
        elif category == "wave":
            data = await LeaderboardRepository.get_top_waves(10)
        elif category == "kd":
            data = await LeaderboardRepository.get_top_kd(10)
        elif category == "bosses":
            data = await LeaderboardRepository.get_top_bosses(10)
        elif category == "playtime":
            data = await LeaderboardRepository.get_top_playtime(10)
        else:
            data = []

        embed = create_leaderboard_embed(data, category, period)
        await interaction.followup.send(embed=embed)

    @app_commands.command(
        name="rank", description="Affiche ton rang dans le classement"
    )
    @app_commands.describe(player="Nom du joueur")
    @app_commands.autocomplete(player=player_autocomplete)
    async def rank(self, interaction: discord.Interaction, player: str):
        """Display player rank."""
        await interaction.response.defer()

        all_time = await LeaderboardRepository.get_player_rank(player, "all")
        weekly = await LeaderboardRepository.get_player_rank(player, "weekly")
        monthly = await LeaderboardRepository.get_player_rank(player, "monthly")

        if all_time is None and weekly is None and monthly is None:
            await interaction.followup.send(
                f"❌ Joueur **{player}** non trouve.", ephemeral=True
            )
            return

        embed = create_rank_embed(player, all_time, weekly, monthly)
        await interaction.followup.send(embed=embed)

    @app_commands.command(
        name="weapon", description="Top 10 par arme specifique"
    )
    @app_commands.describe(weapon="Type d'arme")
    @app_commands.choices(
        weapon=[
            app_commands.Choice(name="Standard", value="standard"),
            app_commands.Choice(name="Spread", value="spread"),
            app_commands.Choice(name="Laser", value="laser"),
            app_commands.Choice(name="Missile", value="missile"),
            app_commands.Choice(name="Wave Cannon", value="waveCannon"),
        ]
    )
    async def weapon(self, interaction: discord.Interaction, weapon: str):
        """Display top players by weapon kills."""
        await interaction.response.defer()

        data = await LeaderboardRepository.get_top_weapon(weapon, 10)

        weapon_names = {
            "standard": "Standard",
            "spread": "Spread",
            "laser": "Laser",
            "missile": "Missile",
            "waveCannon": "Wave Cannon",
        }
        weapon_name = weapon_names.get(weapon, weapon)

        embed = discord.Embed(
            title=f"🔫 TOP 10 - {weapon_name}",
            color=0xFFD700,
        )

        if not data:
            embed.add_field(
                name="Aucune donnee", value="Pas encore de kills!", inline=False
            )
        else:
            lines = []
            emojis = ["🥇", "🥈", "🥉"] + [f"{i}." for i in range(4, 11)]
            field = f"{weapon}Kills"

            for i, entry in enumerate(data):
                emoji = emojis[i] if i < len(emojis) else f"{i+1}."
                name = entry.get("playerName", "Unknown")
                kills = entry.get(field, 0)
                lines.append(f"{emoji} **{name}** | {kills:,} kills")

            embed.add_field(name="Classement", value="\n".join(lines), inline=False)

        await interaction.followup.send(embed=embed)


async def setup(bot: commands.Bot):
    """Setup function for the cog."""
    await bot.add_cog(LeaderboardCog(bot))
