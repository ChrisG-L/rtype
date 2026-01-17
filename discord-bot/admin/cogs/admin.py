"""
Admin commands cog - Server status and management.
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
    parse_status_output,
    parse_sessions_output,
    parse_rooms_output,
    parse_room_details_output,
)

logger = logging.getLogger(__name__)


class AdminCog(commands.Cog):
    """Admin commands for server management."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot
        self._rooms_cache: list[dict[str, str]] = []

    @property
    def tcp(self) -> TCPAdminClient:
        """Get the TCP client from bot."""
        return self.bot.tcp_client

    async def _get_rooms_list(self) -> list[dict[str, str]]:
        """Fetch and cache rooms list for autocomplete."""
        try:
            result = await self.tcp.rooms()
            if result.success:
                self._rooms_cache = parse_rooms_output(result.output)
        except Exception:
            pass
        return self._rooms_cache

    async def room_autocomplete(
        self, interaction: discord.Interaction, current: str
    ) -> list[app_commands.Choice[str]]:
        """Autocomplete for room codes."""
        rooms = await self._get_rooms_list()
        choices = []
        for room in rooms[:25]:
            code = room.get('code', '')
            name = room.get('name', '')
            label = f"{name} ({code})" if name else code
            if current.lower() in code.lower() or current.lower() in name.lower():
                choices.append(app_commands.Choice(name=label, value=code))
        return choices[:25]

    @app_commands.command(name="status", description="Show R-Type server status")
    @is_admin_channel()
    @has_admin_role()
    async def status(self, interaction: discord.Interaction):
        """Show server status."""
        await interaction.response.defer()

        try:
            result = await self.tcp.status()
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Failed to get status")
                )
                return

            # Parse TUI output and create clean embed
            data = parse_status_output(result.output)
            embed = AdminEmbeds.server_status(data)
            await interaction.followup.send(embed=embed)
        except Exception as e:
            logger.error(f"Error getting status: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to get status: {e}")
            )

    @app_commands.command(name="sessions", description="List active sessions")
    @is_admin_channel()
    @has_admin_role()
    async def sessions(self, interaction: discord.Interaction):
        """List active sessions."""
        await interaction.response.defer()

        try:
            result = await self.tcp.sessions()
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Failed to get sessions")
                )
                return

            # Parse TUI output and create clean embed
            sessions_data = parse_sessions_output(result.output)
            embed = AdminEmbeds.sessions_list(sessions_data)
            await interaction.followup.send(embed=embed)
        except Exception as e:
            logger.error(f"Error getting sessions: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to get sessions: {e}")
            )

    @app_commands.command(name="rooms", description="List active game rooms")
    @is_admin_channel()
    @has_admin_role()
    async def rooms(self, interaction: discord.Interaction):
        """List active game rooms."""
        await interaction.response.defer()

        try:
            result = await self.tcp.rooms()
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Failed to get rooms")
                )
                return

            # Parse TUI output and create clean embed
            rooms_data = parse_rooms_output(result.output)
            embed = AdminEmbeds.rooms_list(rooms_data)
            await interaction.followup.send(embed=embed)
        except Exception as e:
            logger.error(f"Error getting rooms: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to get rooms: {e}")
            )

    @app_commands.command(name="room", description="Show details for a specific room")
    @app_commands.describe(room_code="Room code to inspect")
    @app_commands.autocomplete(room_code=room_autocomplete)
    @is_admin_channel()
    @has_admin_role()
    async def room(self, interaction: discord.Interaction, room_code: str):
        """Show room details."""
        await interaction.response.defer()

        try:
            result = await self.tcp.room(room_code)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or f"Failed to get room {room_code}")
                )
                return

            # Parse TUI output and create clean embed
            room_data = parse_room_details_output(result.output)
            if room_data.get('code'):
                embed = AdminEmbeds.room_details(room_data)
            else:
                embed = AdminEmbeds.cli_output(f"Room: {room_code}", result.output)
            await interaction.followup.send(embed=embed)
        except Exception as e:
            logger.error(f"Error getting room: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to get room: {e}")
            )

    @app_commands.command(name="broadcast", description="Broadcast a message to all players")
    @app_commands.describe(message="Message to broadcast")
    @is_admin_channel()
    @has_admin_role()
    async def broadcast(self, interaction: discord.Interaction, message: str):
        """Broadcast a message to all players."""
        await interaction.response.defer()

        try:
            result = await self.tcp.broadcast(message)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Failed to broadcast")
                )
                return

            await interaction.followup.send(
                embed=AdminEmbeds.success(f"Broadcast sent: {message}")
            )
        except Exception as e:
            logger.error(f"Error broadcasting: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to broadcast: {e}")
            )

    @app_commands.command(name="cli", description="Execute any CLI command")
    @app_commands.describe(command="Full CLI command to execute")
    @is_admin_channel()
    @has_admin_role()
    async def cli(self, interaction: discord.Interaction, command: str):
        """Execute a raw CLI command."""
        await interaction.response.defer()

        try:
            # Parse command and args
            parts = command.split(maxsplit=1)
            cmd = parts[0]
            args = parts[1] if len(parts) > 1 else ""

            result = await self.tcp.execute(cmd, args)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Command failed")
                )
                return

            embed = AdminEmbeds.cli_output(f"CLI: {command}", result.output)
            await interaction.followup.send(embed=embed)
        except Exception as e:
            logger.error(f"Error executing CLI command: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Command failed: {e}")
            )

    @app_commands.command(name="help", description="Show admin bot help")
    @is_admin_channel()
    async def help_command(self, interaction: discord.Interaction):
        """Show help for admin commands."""
        embed = discord.Embed(
            title="R-Type Admin Bot - Commands",
            color=0x3498DB,
        )

        commands_list = """
**Server Management**
`/status` - Show server status
`/sessions` - List active sessions
`/rooms` - List active game rooms
`/room <code>` - Show room details

**Communication**
`/broadcast <message>` - Send to all players

**User Management**
`/users` - List all registered users
`/user <email>` - Show user details

**Moderation**
`/kick <email>` - Kick a player (disconnect)
`/ban <email> [reason]` - Ban a user
`/unban <email>` - Unban a user
`/bans` - List all banned users

**Advanced**
`/cli <command>` - Execute any CLI command
"""

        embed.description = commands_list
        embed.set_footer(text="R-Type Admin Bot - Direct TUI Connection")
        await interaction.response.send_message(embed=embed)


async def setup(bot: commands.Bot):
    await bot.add_cog(AdminCog(bot))
