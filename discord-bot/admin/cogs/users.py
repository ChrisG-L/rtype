"""
Users commands cog - User listing and details.
"""

import discord
from discord import app_commands
from discord.ext import commands
import logging

from tcp_client import TCPAdminClient
from database import PlayerStatsRepository
from utils import (
    AdminEmbeds,
    is_admin_channel,
    has_admin_role,
    parse_users_output,
    parse_user_output,
    PaginatedView,
    paginate_items,
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
        except Exception as e:
            logger.debug(f"Failed to fetch users for autocomplete: {e}")
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
    @app_commands.describe(status="Filter by status")
    @app_commands.choices(
        status=[
            app_commands.Choice(name="All", value="all"),
            app_commands.Choice(name="Online", value="online"),
            app_commands.Choice(name="Offline", value="offline"),
            app_commands.Choice(name="Banned", value="banned"),
        ]
    )
    @is_admin_channel()
    @has_admin_role()
    async def users(self, interaction: discord.Interaction, status: str = "all"):
        """List all registered users with pagination."""
        await interaction.response.defer()

        try:
            result = await self.tcp.users()
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Failed to get users")
                )
                return

            # Parse TUI output
            users_data = parse_users_output(result.output)

            # Filter by status if specified
            if status != "all":
                users_data = [u for u in users_data if u.get("status", "").lower() == status.lower()]

            if not users_data:
                filter_names = {"all": "Registered Users", "online": "Online Users", "offline": "Offline Users", "banned": "Banned Users"}
                embed = discord.Embed(
                    title=filter_names.get(status, "Users"),
                    description=f"*No {status} users*" if status != "all" else "*No registered users*",
                    color=0x3498DB,
                )
                await interaction.followup.send(embed=embed)
                return

            # Format user lines for pagination
            lines = []
            for user in users_data:
                user_status = user.get('status', 'Unknown')
                if user_status.lower() == 'online':
                    status_emoji = "🟢"
                elif user_status.lower() == 'banned':
                    status_emoji = "🚫"
                else:
                    status_emoji = "⚫"
                email = user.get('email', 'Unknown')
                username = user.get('username', 'N/A')
                lines.append(f"{status_emoji} **{username}** - `{email}`")

            # Create base embed
            filter_names = {"all": "Registered Users", "online": "Online Users", "offline": "Offline Users", "banned": "Banned Users"}
            base_embed = discord.Embed(
                title=filter_names.get(status, "Registered Users"),
                color=0x3498DB,
            )

            # Paginate if more than 15 users
            if len(lines) > 15:
                pages = paginate_items(lines, base_embed, items_per_page=15, footer_base="R-Type Admin")
                view = PaginatedView(pages, author_id=interaction.user.id)
                await interaction.followup.send(embed=pages[0], view=view)
            else:
                base_embed.description = "\n".join(lines)
                base_embed.set_footer(text=f"Total: {len(lines)} | R-Type Admin")
                await interaction.followup.send(embed=base_embed)
        except Exception as e:
            logger.error(f"Error getting users: {e}", exc_info=True)
            await interaction.followup.send(
                embed=AdminEmbeds.error("Failed to get users. Check logs for details.")
            )

    @app_commands.command(name="user", description="Show details for a specific user")
    @app_commands.describe(email="User email address")
    @app_commands.autocomplete(email=user_autocomplete)
    @is_admin_channel()
    @has_admin_role()
    async def user(self, interaction: discord.Interaction, email: str):
        """Show user details with extended MongoDB stats."""
        await interaction.response.defer()

        try:
            result = await self.tcp.user(email)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or f"Failed to get user {email}")
                )
                return

            # Parse TUI output
            user_data = parse_user_output(result.output)

            # Fetch extended stats from MongoDB
            mongo_stats = await PlayerStatsRepository.get_by_email(email)

            if user_data:
                embed = AdminEmbeds.user_details(user_data, mongo_stats)
            else:
                embed = AdminEmbeds.cli_output(f"User: {email}", result.output)
            await interaction.followup.send(embed=embed)
        except Exception as e:
            logger.error(f"Error getting user: {e}", exc_info=True)
            await interaction.followup.send(
                embed=AdminEmbeds.error("Failed to get user. Check logs for details.")
            )


async def setup(bot: commands.Bot):
    await bot.add_cog(UsersCog(bot))
