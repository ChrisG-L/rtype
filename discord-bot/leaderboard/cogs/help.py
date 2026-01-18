"""
Help command cog - Shows available commands (ephemeral).
"""

import discord
from discord import app_commands
from discord.ext import commands


class HelpCog(commands.Cog):
    """Help command for the leaderboard bot."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot

    @app_commands.command(name="help", description="Show available commands")
    async def help_command(self, interaction: discord.Interaction):
        """Show help for leaderboard commands (ephemeral)."""
        embed = discord.Embed(
            title="R-Type Leaderboard Bot - Commands",
            color=0x00FF00,
        )

        commands_list = """
**Leaderboard**
`/leaderboard [category] [period] [mode] [limit]` - View top players
  - Categories: score, kills, wave, playtime, bosses
  - Periods: all, weekly, monthly, daily
  - Modes: all, solo, duo, trio, 4p, 5p, 6p

**Player Stats**
`/stats <player>` - View player statistics
`/compare <player1> <player2>` - Compare two players
`/history <player> [limit]` - View game history

**Achievements**
`/achievements <player>` - View player achievements with unlock dates

**Server**
`/online` - View currently online players
`/server-stats` - View global server statistics

**Records** *(auto-announced)*
New records are automatically announced when set!
"""

        embed.description = commands_list
        embed.set_footer(text="R-Type Leaderboard Bot")
        await interaction.response.send_message(embed=embed, ephemeral=True)


async def setup(bot: commands.Bot):
    await bot.add_cog(HelpCog(bot))
