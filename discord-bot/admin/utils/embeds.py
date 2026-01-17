"""
Discord embed generators for R-Type Admin Bot.
"""

import discord
from datetime import datetime
from typing import Any
from .formatters import format_datetime, format_duration


class AdminEmbeds:
    """Generator for admin command embeds."""

    # Colors
    COLOR_SUCCESS = 0x00FF00  # Green
    COLOR_ERROR = 0xFF0000    # Red
    COLOR_INFO = 0x3498DB     # Blue
    COLOR_WARNING = 0xFFA500  # Orange

    @staticmethod
    def status(stats: dict[str, Any], uptime: int = 0) -> discord.Embed:
        """Create server status embed."""
        embed = discord.Embed(
            title="R-Type Server Status",
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.utcnow()
        )

        embed.add_field(
            name="Users",
            value=f"Total: **{stats.get('total_users', 0)}**",
            inline=True
        )
        embed.add_field(
            name="Sessions",
            value=f"Active: **{stats.get('active_sessions', 0)}**",
            inline=True
        )
        embed.add_field(
            name="Bans",
            value=f"Banned: **{stats.get('banned_users', 0)}**",
            inline=True
        )

        if uptime > 0:
            embed.add_field(
                name="Uptime",
                value=format_duration(uptime),
                inline=True
            )

        embed.set_footer(text="R-Type Admin")
        return embed

    @staticmethod
    def sessions(sessions: list[dict[str, Any]]) -> discord.Embed:
        """Create sessions list embed."""
        embed = discord.Embed(
            title="Active Sessions",
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.utcnow()
        )

        if not sessions:
            embed.description = "No active sessions"
            return embed

        # Group by email, show first 20
        lines = []
        for i, session in enumerate(sessions[:20]):
            email = session.get("email", "Unknown")
            created = session.get("createdAt")
            created_str = format_datetime(created) if created else "N/A"
            lines.append(f"`{i+1}.` {email} - {created_str}")

        embed.description = "\n".join(lines)

        if len(sessions) > 20:
            embed.set_footer(text=f"Showing 20/{len(sessions)} sessions")
        else:
            embed.set_footer(text=f"Total: {len(sessions)} sessions")

        return embed

    @staticmethod
    def users(users: list[dict[str, Any]]) -> discord.Embed:
        """Create users list embed."""
        embed = discord.Embed(
            title="Registered Users",
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.utcnow()
        )

        if not users:
            embed.description = "No registered users"
            return embed

        lines = []
        for i, user in enumerate(users[:25]):
            email = user.get("email", "Unknown")
            username = user.get("username", "N/A")
            lines.append(f"`{i+1}.` **{username}** ({email})")

        embed.description = "\n".join(lines)

        if len(users) > 25:
            embed.set_footer(text=f"Showing 25/{len(users)} users")
        else:
            embed.set_footer(text=f"Total: {len(users)} users")

        return embed

    @staticmethod
    def user_details(user: dict[str, Any], stats: dict[str, Any] | None = None) -> discord.Embed:
        """Create user details embed."""
        email = user.get("email", "Unknown")
        embed = discord.Embed(
            title=f"User: {user.get('username', 'Unknown')}",
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.utcnow()
        )

        embed.add_field(name="Email", value=email, inline=True)
        embed.add_field(
            name="Created",
            value=format_datetime(user.get("createdAt")),
            inline=True
        )

        if stats:
            embed.add_field(
                name="Games Played",
                value=str(stats.get("gamesPlayed", 0)),
                inline=True
            )
            embed.add_field(
                name="Total Kills",
                value=str(stats.get("totalKills", 0)),
                inline=True
            )
            embed.add_field(
                name="Best Score",
                value=str(stats.get("bestScore", 0)),
                inline=True
            )
            embed.add_field(
                name="Best Wave",
                value=str(stats.get("bestWave", 0)),
                inline=True
            )

        return embed

    @staticmethod
    def bans(banned_users: list[dict[str, Any]]) -> discord.Embed:
        """Create banned users list embed."""
        embed = discord.Embed(
            title="Banned Users",
            color=AdminEmbeds.COLOR_WARNING,
            timestamp=datetime.utcnow()
        )

        if not banned_users:
            embed.description = "No banned users"
            embed.color = AdminEmbeds.COLOR_INFO
            return embed

        lines = []
        for i, ban in enumerate(banned_users[:20]):
            email = ban.get("email", "Unknown")
            reason = ban.get("banReason", "No reason")
            banned_at = format_datetime(ban.get("bannedAt"))
            lines.append(f"`{i+1}.` {email}\n    Reason: {reason}\n    Date: {banned_at}")

        embed.description = "\n".join(lines)

        if len(banned_users) > 20:
            embed.set_footer(text=f"Showing 20/{len(banned_users)} bans")
        else:
            embed.set_footer(text=f"Total: {len(banned_users)} bans")

        return embed

    @staticmethod
    def success(message: str, title: str = "Success") -> discord.Embed:
        """Create success embed."""
        return discord.Embed(
            title=title,
            description=message,
            color=AdminEmbeds.COLOR_SUCCESS,
            timestamp=datetime.utcnow()
        )

    @staticmethod
    def error(message: str, title: str = "Error") -> discord.Embed:
        """Create error embed."""
        return discord.Embed(
            title=title,
            description=message,
            color=AdminEmbeds.COLOR_ERROR,
            timestamp=datetime.utcnow()
        )

    @staticmethod
    def warning(message: str, title: str = "Warning") -> discord.Embed:
        """Create warning embed."""
        return discord.Embed(
            title=title,
            description=message,
            color=AdminEmbeds.COLOR_WARNING,
            timestamp=datetime.utcnow()
        )

    @staticmethod
    def cli_output(title: str, output: list[str]) -> discord.Embed:
        """Create embed for CLI command output."""
        embed = discord.Embed(
            title=title,
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.utcnow()
        )

        if not output:
            embed.description = "*No output*"
        else:
            # Join output lines, limit to 4000 chars (Discord limit is 4096)
            content = "\n".join(output)
            if len(content) > 4000:
                content = content[:3997] + "..."

            # Use code block for CLI output
            embed.description = f"```\n{content}\n```"

        embed.set_footer(text="R-Type Admin")
        return embed
