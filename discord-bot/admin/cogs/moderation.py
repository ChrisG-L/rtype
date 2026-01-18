"""
Moderation commands cog - Ban, unban, kick, tempban, moderation history.
"""

import discord
from discord import app_commands
from discord.ext import commands, tasks
import logging
from datetime import datetime, timedelta, timezone

from tcp_client import TCPAdminClient
from database import ModerationRepository
from utils import (
    AdminEmbeds,
    is_admin_channel,
    has_admin_role,
    parse_bans_output,
    parse_users_output,
    parse_sessions_output,
    PaginatedView,
    paginate_items,
)

logger = logging.getLogger(__name__)

# Duration presets for autocomplete
DURATION_PRESETS = [
    ("15 minutes", 15),
    ("30 minutes", 30),
    ("1 hour", 60),
    ("2 hours", 120),
    ("6 hours", 360),
    ("12 hours", 720),
    ("1 day", 1440),
    ("3 days", 4320),
    ("1 week", 10080),
    ("2 weeks", 20160),
    ("1 month", 43200),
]


class ModerationCog(commands.Cog):
    """Commands for moderation (ban, kick, tempban, history)."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot
        self._users_cache: list[dict[str, str]] = []
        self._bans_cache: list[dict[str, str]] = []
        self._sessions_cache: list[dict[str, str]] = []

    async def cog_load(self):
        """Called when the cog is loaded."""
        self.check_temp_bans.start()
        logger.info("Moderation cog loaded, temp ban checker started")

    async def cog_unload(self):
        """Called when the cog is unloaded."""
        if self.check_temp_bans.is_running():
            self.check_temp_bans.cancel()

    @property
    def tcp(self) -> TCPAdminClient:
        """Get the TCP client from bot."""
        return self.bot.tcp_client

    @tasks.loop(seconds=60)
    async def check_temp_bans(self):
        """Check for expired temp bans and unban them."""
        try:
            expired = await ModerationRepository.get_expired_temp_bans()
            for ban in expired:
                email = ban.get("email")
                if email:
                    # Unban via TCP
                    result = await self.tcp.unban(email)
                    if result.success:
                        await ModerationRepository.mark_temp_ban_unbanned(email)
                        await ModerationRepository.add_mod_action(
                            action="auto_unban",
                            target_email=email,
                            moderator="System",
                            reason="Temp ban expired",
                            details={"original_duration": ban.get("duration_minutes")},
                        )
                        logger.info(f"Auto-unbanned {email} (temp ban expired)")
                    else:
                        logger.error(f"Failed to auto-unban {email}: {result.error}")
        except Exception as e:
            logger.error(f"Error checking temp bans: {e}")

    @check_temp_bans.before_loop
    async def before_check_temp_bans(self):
        """Wait for bot to be ready before starting the loop."""
        await self.bot.wait_until_ready()

    async def _get_users_list(self) -> list[dict[str, str]]:
        """Fetch and cache users list for autocomplete."""
        try:
            result = await self.tcp.users()
            if result.success:
                self._users_cache = parse_users_output(result.output)
        except Exception as e:
            logger.debug(f"Failed to fetch users for autocomplete: {e}")
        return self._users_cache

    async def _get_bans_list(self) -> list[dict[str, str]]:
        """Fetch and cache bans list for autocomplete."""
        try:
            result = await self.tcp.bans()
            if result.success:
                self._bans_cache = parse_bans_output(result.output)
        except Exception as e:
            logger.debug(f"Failed to fetch bans for autocomplete: {e}")
        return self._bans_cache

    async def _get_sessions_list(self) -> list[dict[str, str]]:
        """Fetch and cache sessions list for autocomplete."""
        try:
            result = await self.tcp.sessions()
            if result.success:
                self._sessions_cache = parse_sessions_output(result.output)
        except Exception as e:
            logger.debug(f"Failed to fetch sessions for autocomplete: {e}")
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

    @app_commands.command(name="bans", description="List all banned users")
    @is_admin_channel()
    @has_admin_role()
    async def bans(self, interaction: discord.Interaction):
        """List all banned users with pagination."""
        await interaction.response.defer()

        try:
            result = await self.tcp.bans()
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Failed to get bans")
                )
                return

            # Parse TUI output
            bans_data = parse_bans_output(result.output)

            if not bans_data:
                embed = discord.Embed(
                    title="Banned Users",
                    description="*No banned users*",
                    color=0x3498DB,
                )
                await interaction.followup.send(embed=embed)
                return

            # Format ban lines for pagination
            lines = []
            for ban in bans_data:
                email = ban.get('email', 'Unknown')
                display_name = ban.get('display_name', '-')
                reason = ban.get('reason', '')
                name_part = f" ({display_name})" if display_name and display_name != "-" else ""
                reason_part = f" - *{reason}*" if reason else ""
                lines.append(f"🚫 **{email}**{name_part}{reason_part}")

            # Create base embed
            base_embed = discord.Embed(
                title="Banned Users",
                color=0xFFA500,  # Orange/warning color
            )

            # Paginate if more than 15 bans
            if len(lines) > 15:
                pages = paginate_items(lines, base_embed, items_per_page=15, footer_base="R-Type Admin")
                view = PaginatedView(pages, author_id=interaction.user.id)
                await interaction.followup.send(embed=pages[0], view=view)
            else:
                base_embed.description = "\n".join(lines)
                base_embed.set_footer(text=f"Total: {len(lines)} bans | R-Type Admin")
                await interaction.followup.send(embed=base_embed)
        except Exception as e:
            logger.error(f"Error getting bans: {e}")
            await interaction.followup.send(
                embed=AdminEmbeds.error(f"Failed to get bans: {e}")
            )

    async def duration_autocomplete(
        self, interaction: discord.Interaction, current: str
    ) -> list[app_commands.Choice[str]]:
        """Autocomplete for duration presets."""
        choices = []
        for label, minutes in DURATION_PRESETS:
            if current.lower() in label.lower() or not current:
                choices.append(app_commands.Choice(name=label, value=str(minutes)))
        return choices[:25]

    @app_commands.command(name="tempban", description="Temporarily ban a user")
    @app_commands.describe(
        email="User email to ban",
        duration="Ban duration (e.g., '30' for 30 minutes, or use autocomplete)",
        reason="Reason for ban (optional)"
    )
    @app_commands.autocomplete(email=user_autocomplete, duration=duration_autocomplete)
    @is_admin_channel()
    @has_admin_role()
    async def tempban(
        self,
        interaction: discord.Interaction,
        email: str,
        duration: str,
        reason: str = "",
    ):
        """Temporarily ban a user for a specified duration."""
        await interaction.response.defer()

        try:
            # Parse duration (in minutes)
            try:
                duration_minutes = int(duration)
            except ValueError:
                await interaction.followup.send(
                    embed=AdminEmbeds.error("Invalid duration. Please enter a number (minutes).")
                )
                return

            if duration_minutes < 1:
                await interaction.followup.send(
                    embed=AdminEmbeds.error("Duration must be at least 1 minute.")
                )
                return

            if duration_minutes > 525600:  # 1 year max
                await interaction.followup.send(
                    embed=AdminEmbeds.error("Duration cannot exceed 1 year (525600 minutes).")
                )
                return

            # Calculate expiration
            expires_at = datetime.now(timezone.utc) + timedelta(minutes=duration_minutes)

            # Ban the user via TCP
            result = await self.tcp.ban(email, reason)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or f"Failed to ban {email}")
                )
                return

            # Store temp ban in MongoDB
            await ModerationRepository.add_temp_ban(
                email=email,
                moderator=str(interaction.user),
                reason=reason,
                duration_minutes=duration_minutes,
                expires_at=expires_at,
            )

            # Log moderation action
            await ModerationRepository.add_mod_action(
                action="tempban",
                target_email=email,
                moderator=str(interaction.user),
                reason=reason,
                details={"duration_minutes": duration_minutes, "expires_at": expires_at.isoformat()},
            )

            # Format duration for display
            if duration_minutes < 60:
                duration_str = f"{duration_minutes} minute{'s' if duration_minutes > 1 else ''}"
            elif duration_minutes < 1440:
                hours = duration_minutes // 60
                duration_str = f"{hours} hour{'s' if hours > 1 else ''}"
            else:
                days = duration_minutes // 1440
                duration_str = f"{days} day{'s' if days > 1 else ''}"

            reason_text = f"\n**Reason:** {reason}" if reason else ""
            embed = discord.Embed(
                title="⏱️ User Temporarily Banned",
                description=(
                    f"**{email}** has been banned for **{duration_str}**{reason_text}\n\n"
                    f"**Expires:** <t:{int(expires_at.timestamp())}:F>"
                ),
                color=0xFFA500,
                timestamp=datetime.now(timezone.utc),
            )
            embed.set_footer(text="R-Type Admin")
            await interaction.followup.send(embed=embed)

            logger.info(f"Admin {interaction.user} temp-banned {email} for {duration_minutes}m")
        except Exception as e:
            logger.error(f"Error temp-banning user: {e}", exc_info=True)
            await interaction.followup.send(
                embed=AdminEmbeds.error("Failed to temp-ban user. Check logs for details.")
            )

    @app_commands.command(name="modhistory", description="View moderation history")
    @app_commands.describe(
        user="Filter by target user email (optional)",
        moderator="Filter by moderator (optional)",
        action_type="Filter by action type (optional)",
        limit="Number of entries to show (default: 20)"
    )
    @app_commands.autocomplete(user=user_autocomplete)
    @app_commands.choices(
        action_type=[
            app_commands.Choice(name="All", value="all"),
            app_commands.Choice(name="Ban", value="ban"),
            app_commands.Choice(name="Temp Ban", value="tempban"),
            app_commands.Choice(name="Unban", value="unban"),
            app_commands.Choice(name="Auto Unban", value="auto_unban"),
            app_commands.Choice(name="Kick", value="kick"),
        ]
    )
    @is_admin_channel()
    @has_admin_role()
    async def modhistory(
        self,
        interaction: discord.Interaction,
        user: str = "",
        moderator: str = "",
        action_type: str = "all",
        limit: int = 20,
    ):
        """View moderation history with optional filters."""
        await interaction.response.defer()

        try:
            # Clamp limit
            limit = max(5, min(100, limit))

            # Fetch history
            history = await ModerationRepository.get_mod_history(
                target_email=user if user else None,
                moderator=moderator if moderator else None,
                action=action_type if action_type != "all" else None,
                limit=limit,
            )

            if not history:
                filters = []
                if user:
                    filters.append(f"user: {user}")
                if moderator:
                    filters.append(f"moderator: {moderator}")
                if action_type != "all":
                    filters.append(f"action: {action_type}")
                filter_str = f" ({', '.join(filters)})" if filters else ""

                embed = discord.Embed(
                    title="Moderation History",
                    description=f"*No moderation history found{filter_str}*",
                    color=0x3498DB,
                )
                await interaction.followup.send(embed=embed)
                return

            # Format history lines
            action_emojis = {
                "ban": "🔨",
                "tempban": "⏱️",
                "unban": "✅",
                "auto_unban": "🔓",
                "kick": "👢",
            }

            lines = []
            for entry in history:
                action = entry.get("action", "unknown")
                emoji = action_emojis.get(action, "❓")
                target = entry.get("target_email", "Unknown")
                mod = entry.get("moderator", "Unknown")
                reason = entry.get("reason", "")
                timestamp = entry.get("timestamp")

                # Format timestamp - MongoDB returns naive datetime, treat as UTC
                if timestamp:
                    # If datetime is naive (no timezone), assume UTC
                    if timestamp.tzinfo is None:
                        timestamp = timestamp.replace(tzinfo=timezone.utc)
                    ts_str = f"<t:{int(timestamp.timestamp())}:R>"
                else:
                    ts_str = "Unknown"

                reason_part = f" - *{reason}*" if reason else ""
                lines.append(f"{emoji} **{action.upper()}** `{target}` by {mod}{reason_part} ({ts_str})")

            # Create base embed
            filters = []
            if user:
                filters.append(f"User: {user}")
            if moderator:
                filters.append(f"Mod: {moderator}")
            if action_type != "all":
                filters.append(f"Action: {action_type}")

            filter_title = f" | {', '.join(filters)}" if filters else ""
            base_embed = discord.Embed(
                title=f"Moderation History{filter_title}",
                color=0x3498DB,
            )

            # Paginate if more than 10 entries
            if len(lines) > 10:
                pages = paginate_items(lines, base_embed, items_per_page=10, footer_base="R-Type Admin")
                view = PaginatedView(pages, author_id=interaction.user.id)
                await interaction.followup.send(embed=pages[0], view=view)
            else:
                base_embed.description = "\n".join(lines)
                base_embed.set_footer(text=f"Total: {len(lines)} entries | R-Type Admin")
                await interaction.followup.send(embed=base_embed)

        except Exception as e:
            logger.error(f"Error getting mod history: {e}", exc_info=True)
            await interaction.followup.send(
                embed=AdminEmbeds.error("Failed to get moderation history. Check logs for details.")
            )

    # Override ban/kick/unban to log actions
    @app_commands.command(name="ban", description="Ban a user permanently")
    @app_commands.describe(
        email="User email to ban",
        reason="Reason for ban (optional)"
    )
    @app_commands.autocomplete(email=user_autocomplete)
    @is_admin_channel()
    @has_admin_role()
    async def ban(self, interaction: discord.Interaction, email: str, reason: str = ""):
        """Ban a user permanently."""
        await interaction.response.defer()

        try:
            result = await self.tcp.ban(email, reason)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or f"Failed to ban {email}")
                )
                return

            # Log moderation action
            await ModerationRepository.add_mod_action(
                action="ban",
                target_email=email,
                moderator=str(interaction.user),
                reason=reason,
            )

            reason_text = f"\nReason: {reason}" if reason else ""
            await interaction.followup.send(
                embed=AdminEmbeds.success(f"Banned **{email}**{reason_text}", title="User Banned")
            )
            logger.info(f"Admin {interaction.user} banned {email} (reason: {reason})")
        except Exception as e:
            logger.error(f"Error banning user: {e}", exc_info=True)
            await interaction.followup.send(
                embed=AdminEmbeds.error("Failed to ban user. Check logs for details.")
            )

    @app_commands.command(name="unban", description="Unban a user")
    @app_commands.describe(email="User email to unban")
    @app_commands.autocomplete(email=banned_user_autocomplete)
    @is_admin_channel()
    @has_admin_role()
    async def unban(self, interaction: discord.Interaction, email: str):
        """Unban a user."""
        await interaction.response.defer()

        try:
            result = await self.tcp.unban(email)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or f"Failed to unban {email}")
                )
                return

            # Mark any temp bans as unbanned
            await ModerationRepository.mark_temp_ban_unbanned(email)

            # Log moderation action
            await ModerationRepository.add_mod_action(
                action="unban",
                target_email=email,
                moderator=str(interaction.user),
            )

            await interaction.followup.send(
                embed=AdminEmbeds.success(f"Unbanned **{email}**", title="User Unbanned")
            )
            logger.info(f"Admin {interaction.user} unbanned {email}")
        except Exception as e:
            logger.error(f"Error unbanning user: {e}", exc_info=True)
            await interaction.followup.send(
                embed=AdminEmbeds.error("Failed to unban user. Check logs for details.")
            )

    @app_commands.command(name="kick", description="Kick a player (disconnect from server)")
    @app_commands.describe(email="User email to kick")
    @app_commands.autocomplete(email=session_autocomplete)
    @is_admin_channel()
    @has_admin_role()
    async def kick(self, interaction: discord.Interaction, email: str):
        """Kick a player."""
        await interaction.response.defer()

        try:
            result = await self.tcp.kick(email)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or f"Failed to kick {email}")
                )
                return

            # Log moderation action
            await ModerationRepository.add_mod_action(
                action="kick",
                target_email=email,
                moderator=str(interaction.user),
            )

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
            logger.error(f"Error kicking user: {e}", exc_info=True)
            await interaction.followup.send(
                embed=AdminEmbeds.error("Failed to kick user. Check logs for details.")
            )


async def setup(bot: commands.Bot):
    await bot.add_cog(ModerationCog(bot))
