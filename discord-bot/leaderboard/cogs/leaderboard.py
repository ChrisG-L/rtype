"""
Leaderboard cog - /leaderboard, /rank, /weapon, /daily commands.
"""

import discord
from discord import app_commands
from discord.ext import commands

from database.leaderboard_repo import LeaderboardRepository
from database.player_stats_repo import PlayerStatsRepository
from utils.embeds import create_leaderboard_embed, create_rank_embed
from utils.pagination import PaginatedView, paginate_embed_fields
from utils.formatters import format_number, format_duration

# Constants
ITEMS_PER_PAGE = 10
RANK_EMOJIS = ["🥇", "🥈", "🥉"] + [f"{i}." for i in range(4, 51)]
MODE_NAMES = {0: "Tous", 1: "Solo", 2: "Duo", 3: "Trio", 4: "4P", 5: "5P", 6: "6P"}
MODE_EMOJIS = {1: "👤", 2: "👥", 3: "👥👤", 4: "👥👥", 5: "👥👥👤", 6: "👥👥👥"}


class LeaderboardCog(commands.Cog):
    """Leaderboard commands."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot

    async def player_autocomplete(
        self, interaction: discord.Interaction, current: str
    ) -> list[app_commands.Choice[str]]:
        """Autocomplete for player names."""
        names = await PlayerStatsRepository.search_players(current, limit=25)
        return [app_commands.Choice(name=n, value=n) for n in names]

    @app_commands.command(
        name="leaderboard", description="Affiche le classement des joueurs"
    )
    @app_commands.describe(
        category="Type de classement",
        period="Periode du classement",
        mode="Mode de jeu (nombre de joueurs)",
        limit="Nombre de joueurs a afficher (5-50)",
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
            app_commands.Choice(name="Daily", value="daily"),
            app_commands.Choice(name="Weekly", value="weekly"),
            app_commands.Choice(name="Monthly", value="monthly"),
        ]
    )
    @app_commands.choices(
        mode=[
            app_commands.Choice(name="Tous les modes", value="0"),
            app_commands.Choice(name="Solo", value="1"),
            app_commands.Choice(name="Duo", value="2"),
            app_commands.Choice(name="Trio", value="3"),
            app_commands.Choice(name="4 Joueurs", value="4"),
        ]
    )
    async def leaderboard(
        self,
        interaction: discord.Interaction,
        category: str = "score",
        period: str = "all",
        mode: str = "0",
        limit: int = 10,
    ):
        """Display leaderboard."""
        await interaction.response.defer()

        player_count = int(mode)
        # Clamp limit between 5 and 50
        limit = max(5, min(50, limit))

        if category == "score":
            data = await LeaderboardRepository.get_top_scores(period, limit, player_count)
        elif category == "kills":
            data = await LeaderboardRepository.get_top_kills(limit)
        elif category == "wave":
            data = await LeaderboardRepository.get_top_waves(limit)
        elif category == "kd":
            data = await LeaderboardRepository.get_top_kd(limit)
        elif category == "bosses":
            data = await LeaderboardRepository.get_top_bosses(limit)
        elif category == "playtime":
            data = await LeaderboardRepository.get_top_playtime(limit)
        else:
            data = []

        # Use pagination if more than 10 entries
        if len(data) > ITEMS_PER_PAGE:
            pages = self._create_leaderboard_pages(data, category, period, player_count, limit)
            view = PaginatedView(pages, author_id=interaction.user.id)
            await interaction.followup.send(embed=pages[0], view=view)
        else:
            embed = create_leaderboard_embed(data, category, period, player_count, limit)
            await interaction.followup.send(embed=embed)

    def _create_leaderboard_pages(
        self,
        entries: list[dict],
        category: str,
        period: str,
        player_count: int,
        limit: int,
    ) -> list[discord.Embed]:
        """Create paginated leaderboard embeds."""
        period_names = {
            "all": "All-Time",
            "daily": "Daily",
            "weekly": "Weekly",
            "monthly": "Monthly",
        }
        category_names = {
            "score": "Score",
            "kills": "Kills",
            "wave": "Wave",
            "kd": "K/D Ratio",
            "bosses": "Boss Kills",
            "playtime": "Temps de jeu",
        }

        # Format entries
        lines = []
        for i, entry in enumerate(entries):
            emoji = RANK_EMOJIS[i] if i < len(RANK_EMOJIS) else f"{i+1}."
            name = entry.get("playerName", "Unknown")

            # Mode indicator
            mode_indicator = ""
            if player_count == 0 and category == "score":
                entry_mode = entry.get("playerCount") or 0
                if entry_mode > 0:
                    mode_indicator = f" {MODE_EMOJIS.get(entry_mode, '')}"

            # Format value based on category
            if category == "score":
                value = f"{format_number(entry.get('score', 0))} pts"
            elif category == "kills":
                value = f"{format_number(entry.get('totalKills', 0))} kills"
            elif category == "wave":
                value = f"Wave {entry.get('bestWave', 0)}"
            elif category == "kd":
                kd = entry.get("kdRatio", 0)
                value = f"{kd:.2f} K/D"
            elif category == "bosses":
                value = f"{entry.get('bossKills', 0)} boss"
            elif category == "playtime":
                value = format_duration(entry.get("totalPlaytime", 0))
            else:
                value = "N/A"

            lines.append(f"{emoji} **{name}**{mode_indicator} | {value}")

        # Create base embed
        mode_suffix = f" | {MODE_NAMES.get(player_count, 'Tous')}" if player_count > 0 else ""
        base_embed = discord.Embed(
            title=f"🏆 LEADERBOARD - TOP {limit} ({period_names.get(period, period)}{mode_suffix})",
            description=f"Classement par **{category_names.get(category, category)}**",
            color=0xFFD700,
        )

        return paginate_embed_fields(base_embed, lines, ITEMS_PER_PAGE)

    @app_commands.command(
        name="daily", description="Affiche le top 10 des dernieres 24h"
    )
    @app_commands.describe(
        mode="Mode de jeu (nombre de joueurs)",
    )
    @app_commands.choices(
        mode=[
            app_commands.Choice(name="Tous les modes", value="0"),
            app_commands.Choice(name="Solo", value="1"),
            app_commands.Choice(name="Duo", value="2"),
            app_commands.Choice(name="Trio", value="3"),
            app_commands.Choice(name="4 Joueurs", value="4"),
        ]
    )
    async def daily(
        self,
        interaction: discord.Interaction,
        mode: str = "0",
    ):
        """Display daily top 10 leaderboard (shortcut for /leaderboard daily)."""
        await interaction.response.defer()

        player_count = int(mode)
        data = await LeaderboardRepository.get_top_scores("daily", 10, player_count)

        embed = create_leaderboard_embed(data, "score", "daily", player_count, 10)
        await interaction.followup.send(embed=embed)

    @app_commands.command(
        name="rank", description="Affiche ton rang dans le classement"
    )
    @app_commands.describe(
        player="Nom du joueur",
        mode="Mode de jeu (nombre de joueurs)",
    )
    @app_commands.autocomplete(player=player_autocomplete)
    @app_commands.choices(
        mode=[
            app_commands.Choice(name="Tous les modes", value="0"),
            app_commands.Choice(name="Solo", value="1"),
            app_commands.Choice(name="Duo", value="2"),
            app_commands.Choice(name="Trio", value="3"),
            app_commands.Choice(name="4 Joueurs", value="4"),
        ]
    )
    async def rank(
        self, interaction: discord.Interaction, player: str, mode: str = "0"
    ):
        """Display player rank."""
        await interaction.response.defer()

        player_count = int(mode)

        all_time = await LeaderboardRepository.get_player_rank(player, "all", player_count)
        weekly = await LeaderboardRepository.get_player_rank(player, "weekly", player_count)
        monthly = await LeaderboardRepository.get_player_rank(player, "monthly", player_count)

        if all_time is None and weekly is None and monthly is None:
            await interaction.followup.send(
                f"❌ Joueur **{player}** non trouve.", ephemeral=True
            )
            return

        embed = create_rank_embed(player, all_time, weekly, monthly, player_count)
        await interaction.followup.send(embed=embed)

    @app_commands.command(
        name="weapon", description="Top joueurs par arme specifique"
    )
    @app_commands.describe(
        weapon="Type d'arme",
        limit="Nombre de joueurs a afficher (5-50)",
    )
    @app_commands.choices(
        weapon=[
            app_commands.Choice(name="Standard", value="standard"),
            app_commands.Choice(name="Spread", value="spread"),
            app_commands.Choice(name="Laser", value="laser"),
            app_commands.Choice(name="Missile", value="missile"),
            app_commands.Choice(name="Wave Cannon", value="waveCannon"),
        ]
    )
    async def weapon(self, interaction: discord.Interaction, weapon: str, limit: int = 10):
        """Display top players by weapon kills."""
        await interaction.response.defer()

        # Clamp limit between 5 and 50
        limit = max(5, min(50, limit))

        data = await LeaderboardRepository.get_top_weapon(weapon, limit)

        weapon_names = {
            "standard": "Standard",
            "spread": "Spread",
            "laser": "Laser",
            "missile": "Missile",
            "waveCannon": "Wave Cannon",
        }
        weapon_name = weapon_names.get(weapon, weapon)

        embed = discord.Embed(
            title=f"🔫 TOP {limit} - {weapon_name}",
            color=0xFFD700,
        )

        if not data:
            embed.add_field(
                name="Aucune donnee", value="Pas encore de kills!", inline=False
            )
        else:
            lines = []
            field = f"{weapon}Kills"

            for i, entry in enumerate(data):
                emoji = RANK_EMOJIS[i] if i < len(RANK_EMOJIS) else f"{i+1}."
                name = entry.get("playerName", "Unknown")
                kills = entry.get(field, 0)
                total_kills = entry.get("totalKills", 0)
                # Calculate usage percentage (kills with this weapon / total kills)
                usage_pct = (kills / total_kills * 100) if total_kills > 0 else 0
                lines.append(f"{emoji} **{name}** | {kills:,} kills ({usage_pct:.0f}%)")

            # Use pagination if more than 10 entries
            if len(lines) > ITEMS_PER_PAGE:
                base_embed = discord.Embed(
                    title=f"🔫 TOP {limit} - {weapon_name}",
                    color=0xFFD700,
                )
                base_embed.set_footer(text="% = utilisation de l'arme vs total kills")
                pages = paginate_embed_fields(base_embed, lines, ITEMS_PER_PAGE)
                view = PaginatedView(pages, author_id=interaction.user.id)
                await interaction.followup.send(embed=pages[0], view=view)
                return

            embed.add_field(name="Classement", value="\n".join(lines), inline=False)
            embed.set_footer(text="% = utilisation de l'arme vs total kills")

        await interaction.followup.send(embed=embed)


async def setup(bot: commands.Bot):
    """Setup function for the cog."""
    await bot.add_cog(LeaderboardCog(bot))
