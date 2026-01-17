"""
Users commands cog - User listing and details.
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
    parse_users_output,
    parse_user_output,
)

logger = logging.getLogger(__name__)


class UsersCog(commands.Cog):
    """Commands for user management."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot
        self._users_cache: list[dict[str, str]] = []

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

    async def user_autocomplete(
        self, interaction: discord.Interaction, current: str
    ) -> list[app_commands.Choice[str]]:
        """Autocomplete for user emails."""
        users = await self._get_users_list()
        choices = []
        for user in users[:25]:
            email = user.get('email', '')
            username = user.get('username', '')
            label = f"{username} ({email})" if username else email
            if current.lower() in email.lower() or current.lower() in username.lower():
                choices.append(app_commands.Choice(name=label[:100], value=email))
        return choices[:25]

    @app_commands.command(name="users", description="List all registered users")
    @is_admin_channel()
    @has_admin_role()
    async def users(self, interaction: discord.Interaction):
        """List all registered users."""
        await interaction.response.defer()

        try:
            result = await self.tcp.users()
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Failed to get users")
                )
                return

            # Parse TUI output and create clean embed
            users_data = parse_users_output(result.output)
            embed = AdminEmbeds.users_list(users_data)
            await interaction.followup.send(embed=embed)
        except Exception as e:
            logger.error(f"Error getting users: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to get users: {e}")
            )

    @app_commands.command(name="user", description="Show details for a specific user")
    @app_commands.describe(email="User email address")
    @app_commands.autocomplete(email=user_autocomplete)
    @is_admin_channel()
    @has_admin_role()
    async def user(self, interaction: discord.Interaction, email: str):
        """Show user details."""
        await interaction.response.defer()

        try:
            result = await self.tcp.user(email)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or f"Failed to get user {email}")
                )
                return

            # Parse TUI output and create clean embed
            user_data = parse_user_output(result.output)
            if user_data:
                embed = AdminEmbeds.user_details(user_data)
            else:
                embed = AdminEmbeds.cli_output(f"User: {email}", result.output)
            await interaction.followup.send(embed=embed)
        except Exception as e:
            logger.error(f"Error getting user: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to get user: {e}")
            )


async def setup(bot: commands.Bot):
    await bot.add_cog(UsersCog(bot))
