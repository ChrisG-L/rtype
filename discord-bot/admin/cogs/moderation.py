"""
Moderation commands cog - Ban, unban, kick.
"""

import discord
from discord import app_commands
from discord.ext import commands
import logging

from tcp_client import TCPAdminClient
from utils import (
    AdminEmbeds,
    is_admin_channel,
    has_admin_role,
    parse_bans_output,
    parse_users_output,
    parse_sessions_output,
)

logger = logging.getLogger(__name__)


class ModerationCog(commands.Cog):
    """Commands for moderation (ban, kick, etc.)."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot
        self._users_cache: list[dict[str, str]] = []
        self._bans_cache: list[dict[str, str]] = []
        self._sessions_cache: list[dict[str, str]] = []

    @property
    def tcp(self) -> TCPAdminClient:
        """Get the TCP client from bot."""
        return self.bot.tcp_client

    async def _get_users_list(self) -> list[dict[str, str]]:
        """Fetch and cache users list for autocomplete."""
        try:
            result = await self.tcp.users()
            if result.success:
                self._users_cache = parse_users_output(result.output)
        except Exception:
            pass
        return self._users_cache

    async def _get_bans_list(self) -> list[dict[str, str]]:
        """Fetch and cache bans list for autocomplete."""
        try:
            result = await self.tcp.bans()
            if result.success:
                self._bans_cache = parse_bans_output(result.output)
        except Exception:
            pass
        return self._bans_cache

    async def _get_sessions_list(self) -> list[dict[str, str]]:
        """Fetch and cache sessions list for autocomplete."""
        try:
            result = await self.tcp.sessions()
            if result.success:
                self._sessions_cache = parse_sessions_output(result.output)
        except Exception:
            pass
        return self._sessions_cache

    async def user_autocomplete(
        self, interaction: discord.Interaction, current: str
    ) -> list[app_commands.Choice[str]]:
        """Autocomplete for user emails (all users)."""
        users = await self._get_users_list()
        choices = []
        for user in users[:25]:
            email = user.get('email', '')
            username = user.get('username', '')
            label = f"{username} ({email})" if username else email
            if current.lower() in email.lower() or current.lower() in username.lower():
                choices.append(app_commands.Choice(name=label[:100], value=email))
        return choices[:25]

    async def session_autocomplete(
        self, interaction: discord.Interaction, current: str
    ) -> list[app_commands.Choice[str]]:
        """Autocomplete for online users (active sessions)."""
        sessions = await self._get_sessions_list()
        choices = []
        for session in sessions[:25]:
            email = session.get('email', '')
            display_name = session.get('display_name', '')
            room = session.get('room', '')
            label = f"{display_name} ({email})"
            if room and room != "N/A":
                label += f" - Room: {room}"
            if current.lower() in email.lower() or current.lower() in display_name.lower():
                choices.append(app_commands.Choice(name=label[:100], value=email))
        return choices[:25]

    async def banned_user_autocomplete(
        self, interaction: discord.Interaction, current: str
    ) -> list[app_commands.Choice[str]]:
        """Autocomplete for banned users (for unban)."""
        bans = await self._get_bans_list()
        choices = []
        for ban in bans[:25]:
            email = ban.get('email', '')
            display_name = ban.get('display_name', '')
            label = f"{display_name} ({email})" if display_name and display_name != "-" else email
            if current.lower() in email.lower() or current.lower() in display_name.lower():
                choices.append(app_commands.Choice(name=label[:100], value=email))
        return choices[:25]

    @app_commands.command(name="kick", description="Kick a player (disconnect from server)")
    @app_commands.describe(email="User email to kick")
    @app_commands.autocomplete(email=session_autocomplete)
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
    @app_commands.autocomplete(email=user_autocomplete)
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
    @app_commands.autocomplete(email=banned_user_autocomplete)
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

            # Parse TUI output and create clean embed
            bans_data = parse_bans_output(result.output)
            embed = AdminEmbeds.bans_list(bans_data)
            await interaction.followup.send(embed=embed)
        except Exception as e:
            logger.error(f"Error getting bans: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to get bans: {e}")
            )


async def setup(bot: commands.Bot):
    await bot.add_cog(ModerationCog(bot))
