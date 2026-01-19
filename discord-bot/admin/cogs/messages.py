"""
Private messages admin commands cog.
"""

import discord
from discord import app_commands
from discord.ext import commands
import logging
import re
from datetime import datetime

from tcp_client import TCPAdminClient
from utils import (
    AdminEmbeds,
    is_admin_channel,
    has_admin_role,
)

logger = logging.getLogger(__name__)


def parse_pm_stats_output(output: list[str]) -> dict:
    """Parse pmstats command output."""
    data = {
        "total_messages": 0,
        "total_conversations": 0,
    }

    for line in output:
        if "Total messages:" in line:
            match = re.search(r'(\d+)', line)
            if match:
                data["total_messages"] = int(match.group(1))
        elif "Total conversations:" in line:
            match = re.search(r'(\d+)', line)
            if match:
                data["total_conversations"] = int(match.group(1))

    return data


def parse_pm_output(output: list[str]) -> list[dict]:
    """Parse pmuser/pmconv/pmsearch/pmrecent command output."""
    messages = []
    current_msg = {}

    for line in output:
        # Skip box drawing characters
        if line.startswith('╔') or line.startswith('╠') or line.startswith('╚'):
            continue

        # Parse message header line: [timestamp] status direction email
        header_match = re.match(r'║\s*\[([^\]]+)\]\s*([✓○])\s*([→←]?)\s*(.+)', line)
        if header_match:
            if current_msg:
                messages.append(current_msg)
            current_msg = {
                "timestamp": header_match.group(1),
                "read": header_match.group(2) == "✓",
                "direction": header_match.group(3),
                "party": header_match.group(4).strip().rstrip(':'),
            }
            continue

        # Parse message content line: "content"
        content_match = re.match(r'║\s+"(.+)"', line)
        if content_match and current_msg:
            current_msg["content"] = content_match.group(1)
            continue

    if current_msg and "content" in current_msg:
        messages.append(current_msg)

    return messages


class MessagesCog(commands.Cog):
    """Private messages admin commands."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot

    @property
    def tcp(self) -> TCPAdminClient:
        """Get the TCP client from bot."""
        return self.bot.tcp_client

    @app_commands.command(name="pmstats", description="Show private message statistics")
    @is_admin_channel()
    @has_admin_role()
    async def pmstats(self, interaction: discord.Interaction):
        """Show private message statistics."""
        await interaction.response.defer()

        try:
            result = await self.tcp.execute("pmstats", "")
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Failed to get PM stats")
                )
                return

            data = parse_pm_stats_output(result.output)

            embed = discord.Embed(
                title="📬 Private Message Statistics",
                color=0x3498DB,
            )
            embed.add_field(
                name="Total Messages",
                value=f"**{data['total_messages']:,}**",
                inline=True
            )
            embed.add_field(
                name="Total Conversations",
                value=f"**{data['total_conversations']:,}**",
                inline=True
            )
            embed.timestamp = datetime.now()
            await interaction.followup.send(embed=embed)

        except Exception as e:
            logger.error(f"Error getting PM stats: {e}", exc_info=True)
            await interaction.followup.send(
                embed=AdminEmbeds.error("Failed to get PM stats. Check logs for details.")
            )

    @app_commands.command(name="pmuser", description="Show all messages for a user")
    @app_commands.describe(
        email="User email address",
        limit="Maximum number of messages to show (default: 50)"
    )
    @is_admin_channel()
    @has_admin_role()
    async def pmuser(
        self,
        interaction: discord.Interaction,
        email: str,
        limit: int = 50
    ):
        """Show all messages for a user."""
        await interaction.response.defer()

        try:
            result = await self.tcp.execute("pmuser", f"{email} {limit}")
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or f"Failed to get messages for {email}")
                )
                return

            messages = parse_pm_output(result.output)

            embed = discord.Embed(
                title=f"📧 Messages for {email}",
                color=0x9B59B6,
            )

            if not messages:
                embed.description = "No messages found"
            else:
                lines = []
                for msg in messages[:15]:  # Limit to 15 messages in embed
                    read_icon = "✅" if msg.get("read") else "⬜"
                    direction = msg.get("direction", "")
                    direction_icon = "➡️" if direction == "→" else "⬅️" if direction == "←" else "💬"
                    content = msg.get("content", "")[:50]
                    if len(msg.get("content", "")) > 50:
                        content += "..."

                    lines.append(
                        f"{read_icon} `{msg.get('timestamp', '')}` {direction_icon} **{msg.get('party', '')}**\n"
                        f"   _{content}_"
                    )

                embed.description = "\n\n".join(lines)

                if len(messages) > 15:
                    embed.set_footer(text=f"Showing 15 of {len(messages)} messages")
                else:
                    embed.set_footer(text=f"Total: {len(messages)} messages")

            embed.timestamp = datetime.now()
            await interaction.followup.send(embed=embed)

        except Exception as e:
            logger.error(f"Error getting user messages: {e}", exc_info=True)
            await interaction.followup.send(
                embed=AdminEmbeds.error("Failed to get messages. Check logs for details.")
            )

    @app_commands.command(name="pmconv", description="Show conversation between two users")
    @app_commands.describe(
        email1="First user email",
        email2="Second user email",
        limit="Maximum number of messages (default: 50)"
    )
    @is_admin_channel()
    @has_admin_role()
    async def pmconv(
        self,
        interaction: discord.Interaction,
        email1: str,
        email2: str,
        limit: int = 50
    ):
        """Show conversation between two users."""
        await interaction.response.defer()

        try:
            result = await self.tcp.execute("pmconv", f"{email1} {email2} {limit}")
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Failed to get conversation")
                )
                return

            messages = parse_pm_output(result.output)

            embed = discord.Embed(
                title=f"💬 Conversation: {email1} ↔ {email2}",
                color=0x2ECC71,
            )

            if not messages:
                embed.description = "No messages found"
            else:
                lines = []
                for msg in messages[:20]:  # Limit to 20 messages
                    read_icon = "✅" if msg.get("read") else "⬜"
                    content = msg.get("content", "")[:60]
                    if len(msg.get("content", "")) > 60:
                        content += "..."

                    lines.append(
                        f"{read_icon} `{msg.get('timestamp', '')}` **{msg.get('party', '')}:**\n"
                        f"   _{content}_"
                    )

                embed.description = "\n\n".join(lines)

                if len(messages) > 20:
                    embed.set_footer(text=f"Showing 20 of {len(messages)} messages")
                else:
                    embed.set_footer(text=f"Total: {len(messages)} messages")

            embed.timestamp = datetime.now()
            await interaction.followup.send(embed=embed)

        except Exception as e:
            logger.error(f"Error getting conversation: {e}", exc_info=True)
            await interaction.followup.send(
                embed=AdminEmbeds.error("Failed to get conversation. Check logs for details.")
            )

    @app_commands.command(name="pmsearch", description="Search messages by content")
    @app_commands.describe(search_term="Text to search for in messages")
    @is_admin_channel()
    @has_admin_role()
    async def pmsearch(self, interaction: discord.Interaction, search_term: str):
        """Search messages by content."""
        await interaction.response.defer()

        try:
            result = await self.tcp.execute("pmsearch", search_term)
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Search failed")
                )
                return

            messages = parse_pm_output(result.output)

            embed = discord.Embed(
                title=f"🔍 Search Results: \"{search_term}\"",
                color=0xE74C3C,
            )

            if not messages:
                embed.description = "No messages found matching your search"
            else:
                lines = []
                for msg in messages[:15]:
                    content = msg.get("content", "")[:50]
                    if len(msg.get("content", "")) > 50:
                        content += "..."

                    lines.append(
                        f"`{msg.get('timestamp', '')}` **{msg.get('party', '')}**\n"
                        f"   _{content}_"
                    )

                embed.description = "\n\n".join(lines)
                embed.set_footer(text=f"Found {len(messages)} messages")

            embed.timestamp = datetime.now()
            await interaction.followup.send(embed=embed)

        except Exception as e:
            logger.error(f"Error searching messages: {e}", exc_info=True)
            await interaction.followup.send(
                embed=AdminEmbeds.error("Search failed. Check logs for details.")
            )

    @app_commands.command(name="pmrecent", description="Show most recent private messages")
    @app_commands.describe(limit="Number of messages to show (default: 20, max: 50)")
    @is_admin_channel()
    @has_admin_role()
    async def pmrecent(self, interaction: discord.Interaction, limit: int = 20):
        """Show recent private messages."""
        await interaction.response.defer()

        try:
            # Cap at 50 for performance
            limit = min(limit, 50)

            result = await self.tcp.execute("pmrecent", str(limit))
            if not result.success:
                await interaction.followup.send(
                    embed=AdminEmbeds.error(result.error or "Failed to get recent messages")
                )
                return

            messages = parse_pm_output(result.output)

            embed = discord.Embed(
                title="📥 Recent Private Messages",
                color=0xF39C12,
            )

            if not messages:
                embed.description = "No recent messages"
            else:
                lines = []
                for msg in messages[:15]:
                    read_icon = "✅" if msg.get("read") else "⬜"
                    content = msg.get("content", "")[:40]
                    if len(msg.get("content", "")) > 40:
                        content += "..."

                    lines.append(
                        f"{read_icon} `{msg.get('timestamp', '')}` **{msg.get('party', '')}**\n"
                        f"   _{content}_"
                    )

                embed.description = "\n\n".join(lines)

                if len(messages) > 15:
                    embed.set_footer(text=f"Showing 15 of {len(messages)} messages")
                else:
                    embed.set_footer(text=f"Total: {len(messages)} messages")

            embed.timestamp = datetime.now()
            await interaction.followup.send(embed=embed)

        except Exception as e:
            logger.error(f"Error getting recent messages: {e}", exc_info=True)
            await interaction.followup.send(
                embed=AdminEmbeds.error("Failed to get recent messages. Check logs for details.")
            )


async def setup(bot: commands.Bot):
    await bot.add_cog(MessagesCog(bot))
