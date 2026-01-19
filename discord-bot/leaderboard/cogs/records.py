"""
Records cog - Auto-announce new records in a channel.
Polls MongoDB for new high scores and announces them.
"""

import discord
from discord.ext import commands, tasks
import logging
from datetime import datetime, timezone
from typing import Optional

from config import Config
from database.mongodb import MongoDB
from utils.formatters import format_number, format_duration

logger = logging.getLogger(__name__)

# Record types to track
RECORD_TYPES = {
    "score": {"field": "score", "collection": "leaderboard", "emoji": "🏆", "name": "Score", "unit": "pts"},
    "wave": {"field": "bestWave", "collection": "player_stats", "emoji": "🌊", "name": "Wave", "unit": ""},
    "kills": {"field": "totalKills", "collection": "player_stats", "emoji": "💀", "name": "Kills", "unit": ""},
    "bosses": {"field": "bossKills", "collection": "player_stats", "emoji": "👹", "name": "Boss Kills", "unit": ""},
}


class RecordsCog(commands.Cog):
    """Auto-announce new records and leader changes."""

    def __init__(self, bot: commands.Bot):
        self.bot = bot
        self._last_records: dict[str, int] = {}
        self._last_leader: Optional[str] = None  # Track all-time leader
        self._initialized = False

    async def cog_load(self):
        """Called when the cog is loaded."""
        if Config.RECORDS_CHANNEL_ID:
            # Initialize last known records
            await self._load_current_records()
            # Start the polling task
            self.check_records.start()
            logger.info(f"Records auto-announce enabled (channel: {Config.RECORDS_CHANNEL_ID})")
        else:
            logger.info("Records auto-announce disabled (RECORDS_CHANNEL_ID not set)")

    async def cog_unload(self):
        """Called when the cog is unloaded."""
        if self.check_records.is_running():
            self.check_records.cancel()

    async def _load_current_records(self):
        """Load current top records to avoid announcing old records on startup."""
        db = MongoDB.get()
        if db is None:
            return

        try:
            # Get top score
            top_score = await db.leaderboard.find_one(sort=[("score", -1)])
            if top_score:
                self._last_records["score"] = top_score.get("score", 0)

            # Get top wave
            top_wave = await db.player_stats.find_one(sort=[("bestWave", -1)])
            if top_wave:
                self._last_records["wave"] = top_wave.get("bestWave", 0)

            # Get top kills
            top_kills = await db.player_stats.find_one(sort=[("totalKills", -1)])
            if top_kills:
                self._last_records["kills"] = top_kills.get("totalKills", 0)

            # Get top boss kills
            top_bosses = await db.player_stats.find_one(sort=[("bossKills", -1)])
            if top_bosses:
                self._last_records["bosses"] = top_bosses.get("bossKills", 0)

            # Get current all-time leader
            top_score = await db.leaderboard.find_one(sort=[("score", -1)])
            if top_score:
                self._last_leader = top_score.get("playerName")

            self._initialized = True
            logger.info(f"Loaded current records: {self._last_records}, leader: {self._last_leader}")
        except Exception as e:
            logger.error(f"Failed to load current records: {e}")

    @tasks.loop(seconds=30)  # Default, will be overridden
    async def check_records(self):
        """Check for new records periodically."""
        if not self._initialized:
            return

        db = MongoDB.get()
        if db is None:
            return

        channel = self.bot.get_channel(Config.RECORDS_CHANNEL_ID)
        if channel is None:
            logger.warning(f"Records channel {Config.RECORDS_CHANNEL_ID} not found")
            return

        try:
            # Check for new top score (also checks leader change)
            await self._check_score_record(db, channel)

            # Check for new top wave
            await self._check_stat_record(db, channel, "wave", "bestWave")

            # Check for new top kills
            await self._check_stat_record(db, channel, "kills", "totalKills")

            # Check for new top boss kills
            await self._check_stat_record(db, channel, "bosses", "bossKills")

            # Check for leader change (separate from record)
            await self._check_leader_change(db, channel)

        except Exception as e:
            logger.error(f"Error checking records: {e}")

    @check_records.before_loop
    async def before_check_records(self):
        """Wait for bot to be ready before starting the loop."""
        await self.bot.wait_until_ready()
        # Update loop interval from config
        self.check_records.change_interval(seconds=Config.RECORDS_POLL_INTERVAL)

    async def _check_score_record(self, db, channel: discord.TextChannel):
        """Check for new score record."""
        top = await db.leaderboard.find_one(sort=[("score", -1)])
        if not top:
            return

        current_best = top.get("score", 0)
        last_best = self._last_records.get("score", 0)

        if current_best > last_best:
            self._last_records["score"] = current_best
            await self._announce_record(
                channel,
                record_type="score",
                player_name=top.get("playerName", "Unknown"),
                new_value=current_best,
                old_value=last_best,
                extra_info={
                    "wave": top.get("wave", 0),
                    "kills": top.get("kills", 0),
                    "duration": top.get("duration", 0),
                    "playerCount": top.get("playerCount", 1),
                },
            )

    async def _check_stat_record(
        self, db, channel: discord.TextChannel, record_key: str, field: str
    ):
        """Check for new stat record (wave, kills, bosses)."""
        top = await db.player_stats.find_one(sort=[(field, -1)])
        if not top:
            return

        current_best = top.get(field, 0)
        last_best = self._last_records.get(record_key, 0)

        if current_best > last_best:
            self._last_records[record_key] = current_best
            await self._announce_record(
                channel,
                record_type=record_key,
                player_name=top.get("playerName", "Unknown"),
                new_value=current_best,
                old_value=last_best,
            )

    async def _check_leader_change(self, db, channel: discord.TextChannel):
        """Check if the all-time leader has changed (without breaking the record)."""
        top = await db.leaderboard.find_one(sort=[("score", -1)])
        if not top:
            return

        current_leader = top.get("playerName")
        current_score = top.get("score", 0)

        # Only announce if leader changed AND it's not a new record
        # (new records are announced separately with more details)
        if current_leader and current_leader != self._last_leader:
            old_leader = self._last_leader
            self._last_leader = current_leader

            # Skip if this is the first initialization or if it's a new record
            if old_leader is None:
                return

            # Check if this was already announced as a new record
            if current_score > self._last_records.get("score", 0):
                # This will be/was announced as a new record, skip leader announcement
                return

            await self._announce_leader_change(
                channel,
                new_leader=current_leader,
                old_leader=old_leader,
                score=current_score,
                extra_info={
                    "wave": top.get("wave", 0),
                    "kills": top.get("kills", 0),
                    "playerCount": top.get("playerCount", 1),
                },
            )

    async def _announce_leader_change(
        self,
        channel: discord.TextChannel,
        new_leader: str,
        old_leader: str,
        score: int,
        extra_info: Optional[dict] = None,
    ):
        """Announce a new all-time leader."""
        embed = discord.Embed(
            title="👑 NEW ALL-TIME LEADER! 👑",
            description=f"**{new_leader}** has taken the #1 spot!",
            color=0xFFD700,  # Gold
            timestamp=datetime.now(timezone.utc),
        )

        embed.add_field(name="New Leader", value=f"**{new_leader}**", inline=True)
        embed.add_field(name="Previous Leader", value=old_leader, inline=True)
        embed.add_field(name="Score", value=f"**{format_number(score)}** pts", inline=True)

        if extra_info:
            mode_names = {1: "Solo", 2: "Duo", 3: "Trio", 4: "4P"}
            mode = mode_names.get(extra_info.get("playerCount", 1), "?")
            details = (
                f"**Wave:** {extra_info.get('wave', 0)}\n"
                f"**Kills:** {extra_info.get('kills', 0)}\n"
                f"**Mode:** {mode}"
            )
            embed.add_field(name="Game Details", value=details, inline=False)

        embed.set_footer(text="R-Type Leaderboard")

        try:
            await channel.send(embed=embed)
            logger.info(f"Announced new leader: {new_leader} (was: {old_leader})")
        except discord.Forbidden:
            logger.error(f"Cannot send messages to records channel {channel.id}")
        except Exception as e:
            logger.error(f"Failed to announce leader change: {e}")

    async def _announce_record(
        self,
        channel: discord.TextChannel,
        record_type: str,
        player_name: str,
        new_value: int,
        old_value: int,
        extra_info: Optional[dict] = None,
    ):
        """Announce a new record in the channel."""
        record_info = RECORD_TYPES.get(record_type, {})
        emoji = record_info.get("emoji", "🏆")
        name = record_info.get("name", record_type.title())
        unit = record_info.get("unit", "")

        # Calculate improvement
        improvement = new_value - old_value
        improvement_pct = (improvement / old_value * 100) if old_value > 0 else 0

        # Create embed
        embed = discord.Embed(
            title=f"{emoji} NEW RECORD! {emoji}",
            description=f"**{player_name}** has set a new **{name}** record!",
            color=0xFFD700,  # Gold
            timestamp=datetime.now(timezone.utc),
        )

        # Format value based on type
        if record_type == "score":
            value_str = f"{format_number(new_value)} {unit}"
        else:
            value_str = f"{new_value:,} {unit}".strip()

        embed.add_field(name=f"New {name}", value=f"**{value_str}**", inline=True)

        if old_value > 0:
            if record_type == "score":
                old_str = f"{format_number(old_value)} {unit}"
            else:
                old_str = f"{old_value:,} {unit}".strip()
            embed.add_field(name="Previous", value=old_str, inline=True)
            embed.add_field(name="Improvement", value=f"+{improvement_pct:.1f}%", inline=True)

        # Extra info for score records
        if extra_info and record_type == "score":
            mode_names = {1: "Solo", 2: "Duo", 3: "Trio", 4: "4P"}
            mode = mode_names.get(extra_info.get("playerCount", 1), "?")
            details = (
                f"**Wave:** {extra_info.get('wave', 0)}\n"
                f"**Kills:** {extra_info.get('kills', 0)}\n"
                f"**Duration:** {format_duration(extra_info.get('duration', 0))}\n"
                f"**Mode:** {mode}"
            )
            embed.add_field(name="Game Details", value=details, inline=False)

        embed.set_footer(text="R-Type Leaderboard")

        try:
            await channel.send(embed=embed)
            logger.info(f"Announced new {record_type} record: {player_name} = {new_value}")
        except discord.Forbidden:
            logger.error(f"Cannot send messages to records channel {channel.id}")
        except Exception as e:
            logger.error(f"Failed to announce record: {e}")


async def setup(bot: commands.Bot):
    """Setup function for the cog."""
    await bot.add_cog(RecordsCog(bot))
