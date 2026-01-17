"""
Moderation commands cog - Ban, unban, kick.
"""

import discord
from discord import app_commands
from discord.ext import commands
import logging

from tcp_client import TCPAdminClient
from utils import AdminEmbeds, is_admin_channel, has_admin_role

logger = logging.getLogger(__name__)


class ModerationCog(commands.Cog):
    """Commands for moderation (ban, kick, etc.)."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot

    @property
    def tcp(self) -> TCPAdminClient:
        """Get the TCP client from bot."""
        return self.bot.tcp_client

    @app_commands.command(name="kick", description="Kick a player (disconnect from server)")
    @app_commands.describe(email="User email to kick")
    @is_admin_channel()
    @has_admin_role()
    async def kick(self, interaction: discord.Interaction, email: str):
        """Kick a player by email."""
        await interaction.response.defer()

        try:
            result = await self.tcp.kick(email)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or f"Failed to kick {email}")
                )
                return

            # Check output for success message
            output_text = "\n".join(result.output)
            if "kicked" in output_text.lower() or "disconnected" in output_text.lower():
                await interaction.followup.send(
                    embed=AdminEmbeds.success(f"Kicked **{email}**", title="Player Kicked")
                )
            else:
                embed = AdminEmbeds.cli_output(f"Kick: {email}", result.output)
                await interaction.followup.send(embed=embed)

            logger.info(f"Admin {interaction.user} kicked {email}")
        except Exception as e:
            logger.error(f"Error kicking user: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to kick user: {e}")
            )

    @app_commands.command(name="ban", description="Ban a user")
    @app_commands.describe(
        email="User email to ban",
        reason="Reason for ban (optional)"
    )
    @is_admin_channel()
    @has_admin_role()
    async def ban(self, interaction: discord.Interaction, email: str, reason: str = ""):
        """Ban a user by email."""
        await interaction.response.defer()

        try:
            result = await self.tcp.ban(email, reason)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or f"Failed to ban {email}")
                )
                return

            reason_text = f"\nReason: {reason}" if reason else ""
            await interaction.followup.send(
                embed=AdminEmbeds.success(f"Banned **{email}**{reason_text}", title="User Banned")
            )
            logger.info(f"Admin {interaction.user} banned {email} (reason: {reason})")
        except Exception as e:
            logger.error(f"Error banning user: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to ban user: {e}")
            )

    @app_commands.command(name="unban", description="Unban a user")
    @app_commands.describe(email="User email to unban")
    @is_admin_channel()
    @has_admin_role()
    async def unban(self, interaction: discord.Interaction, email: str):
        """Unban a user by email."""
        await interaction.response.defer()

        try:
            result = await self.tcp.unban(email)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or f"Failed to unban {email}")
                )
                return

            await interaction.followup.send(
                embed=AdminEmbeds.success(f"Unbanned **{email}**", title="User Unbanned")
            )
            logger.info(f"Admin {interaction.user} unbanned {email}")
        except Exception as e:
            logger.error(f"Error unbanning user: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to unban user: {e}")
            )

    @app_commands.command(name="bans", description="List all banned users")
    @is_admin_channel()
    @has_admin_role()
    async def bans(self, interaction: discord.Interaction):
        """List all banned users."""
        await interaction.response.defer()

        try:
            result = await self.tcp.bans()
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Failed to get bans")
                )
                return

            embed = AdminEmbeds.cli_output("Banned Users", result.output)
            await interaction.followup.send(embed=embed)
        except Exception as e:
            logger.error(f"Error getting bans: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to get bans: {e}")
            )


async def setup(bot: commands.Bot):
    await bot.add_cog(ModerationCog(bot))
