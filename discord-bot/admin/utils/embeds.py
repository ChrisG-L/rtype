"""
Discord embed generators for R-Type Admin Bot.
"""

import discord
from datetime import datetime, timezone
from typing import Any
from .formatters import format_datetime, format_duration


class AdminEmbeds:
    """Generator for admin command embeds."""

    # Colors
    COLOR_SUCCESS = 0x00FF00  # Green
    COLOR_ERROR = 0xFF0000    # Red
    COLOR_INFO = 0x3498DB     # Blue
    COLOR_WARNING = 0xFFA500  # Orange
    COLOR_ONLINE = 0x2ECC71   # Green for online
    COLOR_OFFLINE = 0x95A5A6  # Gray for offline

    @staticmethod
    def server_status(data: dict[str, Any]) -> discord.Embed:
        """Create server status embed with parsed data."""
        embed = discord.Embed(
            title="R-Type Server Status",
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.now(timezone.utc)
        )

        # Main stats
        embed.add_field(
            name="Sessions",
            value=f"**{data.get('active_sessions', 0)}** active",
            inline=True
        )
        embed.add_field(
            name="Players",
            value=f"**{data.get('players_in_game', 0)}** in game",
            inline=True
        )
        embed.add_field(
            name="Rooms",
            value=f"**{data.get('active_rooms', 0)}** active",
            inline=True
        )

        # User stats
        embed.add_field(
            name="Users",
            value=f"**{data.get('users_in_db', 0)}** registered",
            inline=True
        )
        embed.add_field(
            name="Banned",
            value=f"**{data.get('banned_users', 0)}** users",
            inline=True
        )

        # Server config
        logs_emoji = "\u2705" if data.get('logs') == 'ON' else "\u274C"
        debug_emoji = "\u2705" if data.get('debug') == 'ON' else "\u274C"
        embed.add_field(
            name="Config",
            value=f"Logs: {logs_emoji} | Debug: {debug_emoji}",
            inline=True
        )

        embed.set_footer(text="R-Type Admin")
        return embed

    @staticmethod
    def users_list(users: list[dict[str, str]], status_filter: str = "all") -> discord.Embed:
        """Create users list embed with parsed data."""
        # Title based on filter
        filter_names = {"all": "Registered Users", "online": "Online Users", "offline": "Offline Users", "banned": "Banned Users"}
        title = filter_names.get(status_filter, "Registered Users")

        embed = discord.Embed(
            title=title,
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.now(timezone.utc)
        )

        if not users:
            embed.description = f"*No {status_filter} users*" if status_filter != "all" else "*No registered users*"
            return embed

        lines = []
        for i, user in enumerate(users[:25], 1):
            status = user.get('status', 'Unknown')
            if status.lower() == 'online':
                status_emoji = "\U0001F7E2"  # 🟢
            elif status.lower() == 'banned':
                status_emoji = "\U0001F6AB"  # 🚫
            else:
                status_emoji = "\u26AB"  # ⚫
            email = user.get('email', 'Unknown')
            username = user.get('username', 'N/A')
            lines.append(f"{status_emoji} **{username}** - `{email}`")

        embed.description = "\n".join(lines)

        filter_text = f" ({status_filter})" if status_filter != "all" else ""
        if len(users) > 25:
            embed.set_footer(text=f"Showing 25/{len(users)} users{filter_text} | R-Type Admin")
        else:
            embed.set_footer(text=f"Total: {len(users)} users{filter_text} | R-Type Admin")

        return embed

    @staticmethod
    def sessions_list(sessions: list[dict[str, str]]) -> discord.Embed:
        """Create sessions list embed with parsed data."""
        embed = discord.Embed(
            title="Active Sessions",
            color=AdminEmbeds.COLOR_ONLINE if sessions else AdminEmbeds.COLOR_INFO,
            timestamp=datetime.now(timezone.utc)
        )

        if not sessions:
            embed.description = "*No active sessions*"
            return embed

        lines = []
        for session in sessions[:20]:
            email = session.get('email', 'Unknown')
            display_name = session.get('display_name', '')
            status = session.get('status', 'Unknown')
            room = session.get('room', 'N/A')

            # Status emoji
            if status == "Active":
                emoji = "\U0001F7E2"  # 🟢
            elif status == "Pending":
                emoji = "\U0001F7E1"  # 🟡
            else:
                emoji = "\u26AB"  # ⚫

            name_part = f" ({display_name})" if display_name else ""
            room_part = f" - Room: `{room}`" if room != "N/A" else ""
            lines.append(f"{emoji} **{email}**{name_part}{room_part}")

        embed.description = "\n".join(lines)
        embed.set_footer(text=f"Total: {len(sessions)} sessions | R-Type Admin")

        return embed

    @staticmethod
    def rooms_list(rooms: list[dict[str, str]]) -> discord.Embed:
        """Create rooms list embed with parsed data."""
        embed = discord.Embed(
            title="Active Game Rooms",
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.now(timezone.utc)
        )

        if not rooms:
            embed.description = "*No active rooms*"
            return embed

        lines = []
        for room in rooms[:20]:
            code = room.get('code', 'Unknown')
            name = room.get('name', '')
            players = room.get('players', '0')
            status = room.get('status', 'Unknown')
            host = room.get('host', '')

            # Status emoji
            if status == "InGame":
                emoji = "\U0001F3AE"  # 🎮
            elif status == "Waiting":
                emoji = "\u23F3"  # ⏳
            elif status == "Starting":
                emoji = "\U0001F680"  # 🚀
            else:
                emoji = "\U0001F3AE"

            line = f"{emoji} **{name}** (`{code}`) - {players} ({status})"
            if host:
                line += f" - Host: `{host}`"
            lines.append(line)

        embed.description = "\n".join(lines)
        embed.set_footer(text=f"Total: {len(rooms)} rooms | R-Type Admin")

        return embed

    @staticmethod
    def bans_list(bans: list[dict[str, str]]) -> discord.Embed:
        """Create banned users list embed with parsed data."""
        embed = discord.Embed(
            title="Banned Users",
            color=AdminEmbeds.COLOR_WARNING if bans else AdminEmbeds.COLOR_INFO,
            timestamp=datetime.now(timezone.utc)
        )

        if not bans:
            embed.description = "*No banned users*"
            return embed

        lines = []
        for ban in bans[:20]:
            email = ban.get('email', 'Unknown')
            display_name = ban.get('display_name', '-')
            name_part = f" ({display_name})" if display_name and display_name != "-" else ""
            lines.append(f"\U0001F6AB **{email}**{name_part}")

        embed.description = "\n".join(lines)
        embed.set_footer(text=f"Total: {len(bans)} bans | R-Type Admin")

        return embed

    @staticmethod
    def user_details(user: dict[str, str], mongo_stats: dict | None = None) -> discord.Embed:
        """Create user details embed with parsed data and MongoDB stats."""
        username = user.get('username', 'Unknown')
        status = user.get('status', 'Unknown')

        color = AdminEmbeds.COLOR_ONLINE if status.lower() == 'online' else AdminEmbeds.COLOR_OFFLINE
        embed = discord.Embed(
            title=f"User: {username}",
            color=color,
            timestamp=datetime.now(timezone.utc)
        )

        # Status with emoji
        status_emoji = "\U0001F7E2" if status.lower() == 'online' else "\u26AB"
        embed.add_field(name="Status", value=f"{status_emoji} {status}", inline=True)

        if user.get('email'):
            embed.add_field(name="Email", value=f"`{user['email']}`", inline=True)

        if user.get('created'):
            embed.add_field(name="Created", value=user['created'], inline=True)

        # MongoDB extended stats
        if mongo_stats:
            # Games & Score
            games = mongo_stats.get('gamesPlayed', 0)
            best_score = mongo_stats.get('bestScore', 0)
            total_score = mongo_stats.get('totalScore', 0)
            embed.add_field(name="Games Played", value=f"**{games:,}**", inline=True)
            embed.add_field(name="Best Score", value=f"**{best_score:,}**", inline=True)
            embed.add_field(name="Total Score", value=f"**{total_score:,}**", inline=True)

            # Combat stats
            kills = mongo_stats.get('totalKills', 0)
            deaths = mongo_stats.get('totalDeaths', 0)
            kd_ratio = kills / deaths if deaths > 0 else kills
            embed.add_field(name="Kills", value=f"**{kills:,}**", inline=True)
            embed.add_field(name="Deaths", value=f"**{deaths:,}**", inline=True)
            embed.add_field(name="K/D Ratio", value=f"**{kd_ratio:.2f}**", inline=True)

            # Best records
            best_wave = mongo_stats.get('bestWave', 0)
            best_combo = mongo_stats.get('bestCombo', 0) / 10.0 if mongo_stats.get('bestCombo') else 0
            boss_kills = mongo_stats.get('bossKills', 0)
            embed.add_field(name="Best Wave", value=f"**{best_wave}**", inline=True)
            embed.add_field(name="Best Combo", value=f"**{best_combo:.1f}x**", inline=True)
            embed.add_field(name="Boss Kills", value=f"**{boss_kills:,}**", inline=True)

            # Playtime
            playtime_secs = mongo_stats.get('totalPlaytime', 0)
            hours = playtime_secs // 3600
            minutes = (playtime_secs % 3600) // 60
            playtime_str = f"{hours}h {minutes}m" if hours > 0 else f"{minutes}m"
            embed.add_field(name="Playtime", value=f"**{playtime_str}**", inline=True)

            # Weapon kills breakdown
            standard = mongo_stats.get('standardKills', 0)
            spread = mongo_stats.get('spreadKills', 0)
            laser = mongo_stats.get('laserKills', 0)
            missile = mongo_stats.get('missileKills', 0)
            weapon_str = f"Standard: {standard:,} | Spread: {spread:,}\nLaser: {laser:,} | Missile: {missile:,}"
            embed.add_field(name="Weapon Kills", value=weapon_str, inline=False)
        else:
            # Fallback to TUI stats if available
            if user.get('games_played'):
                embed.add_field(name="Games Played", value=user['games_played'], inline=True)
            if user.get('best_score'):
                embed.add_field(name="Best Score", value=user['best_score'], inline=True)
            if user.get('total_kills'):
                embed.add_field(name="Total Kills", value=user['total_kills'], inline=True)

        embed.set_footer(text="R-Type Admin")
        return embed

    @staticmethod
    def room_details(room: dict[str, Any]) -> discord.Embed:
        """Create room details embed with parsed data."""
        name = room.get('name', 'Unknown')
        code = room.get('code', 'Unknown')
        state = room.get('state', 'Unknown')

        # Color based on state
        if state == "InGame":
            color = AdminEmbeds.COLOR_ONLINE
        elif state == "Waiting":
            color = AdminEmbeds.COLOR_INFO
        else:
            color = AdminEmbeds.COLOR_WARNING

        embed = discord.Embed(
            title=f"\U0001F3AE Room: {name}",
            color=color,
            timestamp=datetime.now(timezone.utc)
        )

        # Room info
        embed.add_field(name="Code", value=f"`{code}`", inline=True)
        embed.add_field(name="Players", value=room.get('players', '0/4'), inline=True)

        # State emoji
        if state == "InGame":
            state_display = "\U0001F3AE InGame"
        elif state == "Waiting":
            state_display = "\u23F3 Waiting"
        elif state == "Starting":
            state_display = "\U0001F680 Starting"
        else:
            state_display = state
        embed.add_field(name="State", value=state_display, inline=True)

        embed.add_field(name="Host", value=f"`{room.get('host', 'N/A')}`", inline=True)

        private = room.get('private', 'No')
        private_emoji = "\U0001F512" if private == "Yes" else "\U0001F513"
        embed.add_field(name="Private", value=f"{private_emoji} {private}", inline=True)

        # Players list
        player_list = room.get('player_list', [])
        if player_list:
            lines = []
            for p in player_list:
                ready_emoji = "\u2705" if p.get('ready') == "Yes" else "\u274C"
                host_emoji = "\U0001F451" if p.get('is_host') == "Yes" else ""
                display = p.get('display_name', 'Unknown')
                lines.append(f"{ready_emoji} {host_emoji} **{display}**")

            embed.add_field(
                name=f"Players ({len(player_list)})",
                value="\n".join(lines),
                inline=False
            )

        embed.set_footer(text="R-Type Admin")
        return embed

    # Keep old methods for backward compatibility

    @staticmethod
    def status(stats: dict[str, Any], uptime: int = 0) -> discord.Embed:
        """Create server status embed (legacy)."""
        embed = discord.Embed(
            title="R-Type Server Status",
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.now(timezone.utc)
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
        """Create sessions list embed (legacy)."""
        embed = discord.Embed(
            title="Active Sessions",
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.now(timezone.utc)
        )

        if not sessions:
            embed.description = "No active sessions"
            return embed

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
        """Create users list embed (legacy)."""
        embed = discord.Embed(
            title="Registered Users",
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.now(timezone.utc)
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
    def bans(banned_users: list[dict[str, Any]]) -> discord.Embed:
        """Create banned users list embed (legacy)."""
        embed = discord.Embed(
            title="Banned Users",
            color=AdminEmbeds.COLOR_WARNING,
            timestamp=datetime.now(timezone.utc)
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
            title=f"\u2705 {title}",
            description=message,
            color=AdminEmbeds.COLOR_SUCCESS,
            timestamp=datetime.now(timezone.utc)
        )

    @staticmethod
    def error(message: str, title: str = "Error") -> discord.Embed:
        """Create error embed."""
        return discord.Embed(
            title=f"\u274C {title}",
            description=message,
            color=AdminEmbeds.COLOR_ERROR,
            timestamp=datetime.now(timezone.utc)
        )

    @staticmethod
    def warning(message: str, title: str = "Warning") -> discord.Embed:
        """Create warning embed."""
        return discord.Embed(
            title=f"\u26A0\uFE0F {title}",
            description=message,
            color=AdminEmbeds.COLOR_WARNING,
            timestamp=datetime.now(timezone.utc)
        )

    @staticmethod
    def server_info(status_data: dict, mongo_stats: dict, bot_user) -> discord.Embed:
        """Create server info embed with extended MongoDB statistics."""
        embed = discord.Embed(
            title="R-Type Server Information",
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.now(timezone.utc)
        )

        # Server status section
        if status_data:
            sessions = status_data.get('active_sessions', 0)
            players = status_data.get('players_in_game', 0)
            rooms = status_data.get('active_rooms', 0)
            users = status_data.get('users_in_db', 0)
            banned = status_data.get('banned_users', 0)

            status_text = (
                f"**Sessions:** {sessions} active\n"
                f"**Players:** {players} in game\n"
                f"**Rooms:** {rooms} active\n"
                f"**Users:** {users} registered\n"
                f"**Banned:** {banned} users"
            )
            embed.add_field(name="Server Status", value=status_text, inline=True)
        else:
            embed.add_field(name="Server Status", value="*Unavailable*", inline=True)

        # MongoDB global stats
        if mongo_stats:
            total_games = mongo_stats.get('total_games', 0)
            total_kills = mongo_stats.get('total_kills', 0)
            playtime_secs = mongo_stats.get('total_playtime', 0)
            players_tracked = mongo_stats.get('players_with_stats', 0)

            # Format playtime
            hours = playtime_secs // 3600
            playtime_str = f"{hours:,}h" if hours > 0 else "0h"

            stats_text = (
                f"**Total Games:** {total_games:,}\n"
                f"**Total Kills:** {total_kills:,}\n"
                f"**Total Playtime:** {playtime_str}\n"
                f"**Players Tracked:** {players_tracked:,}"
            )
            embed.add_field(name="Global Statistics", value=stats_text, inline=True)

            # Top player
            top_score = mongo_stats.get('top_score', 0)
            top_player = mongo_stats.get('top_player', 'N/A')
            if top_score > 0:
                embed.add_field(
                    name="Top Score",
                    value=f"**{top_player}**\n{top_score:,} pts",
                    inline=True
                )
        else:
            embed.add_field(name="Global Statistics", value="*MongoDB unavailable*", inline=True)

        # Bot info
        if bot_user:
            embed.set_thumbnail(url=bot_user.display_avatar.url)

        embed.set_footer(text="R-Type Admin Bot")
        return embed

    @staticmethod
    def cli_output(title: str, output: list[str]) -> discord.Embed:
        """Create embed for CLI command output."""
        embed = discord.Embed(
            title=title,
            color=AdminEmbeds.COLOR_INFO,
            timestamp=datetime.now(timezone.utc)
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
