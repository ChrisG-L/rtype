"""Discord embed generators for R-Type Bot."""

import discord
from typing import Optional

from .formatters import (
    format_number,
    format_duration,
    format_timestamp,
    format_game_duration,
    progress_bar,
)
from models.leaderboard_entry import LeaderboardEntry
from models.player_stats import PlayerStats
from models.game_history import GameHistoryEntry
from models.achievement import Achievement

# Rank emojis
RANK_EMOJIS = ["🥇", "🥈", "🥉"] + [f"{i}." for i in range(4, 11)]

# Weapon emojis
WEAPON_EMOJIS = {
    "Standard": "🔵",
    "Spread": "🟢",
    "Laser": "🔴",
    "Missile": "🟡",
    "Wave Cannon": "🟣",
}

# Colors
GOLD = 0xFFD700
GREEN = 0x00FF00
RED = 0xFF4444
BLUE = 0x4444FF


def create_leaderboard_embed(
    entries: list[dict],
    category: str,
    period: str,
) -> discord.Embed:
    """Create leaderboard embed."""
    period_names = {"all": "All-Time", "weekly": "Weekly", "monthly": "Monthly"}
    category_names = {
        "score": "Score",
        "kills": "Kills",
        "wave": "Wave",
        "kd": "K/D Ratio",
        "bosses": "Boss Kills",
        "playtime": "Temps de jeu",
    }

    embed = discord.Embed(
        title=f"🏆 LEADERBOARD - TOP 10 ({period_names.get(period, period)})",
        description=f"Classement par **{category_names.get(category, category)}**",
        color=GOLD,
    )

    if not entries:
        embed.add_field(
            name="Aucune donnee", value="Personne n'a encore joue!", inline=False
        )
        return embed

    lines = []
    for i, entry in enumerate(entries):
        emoji = RANK_EMOJIS[i] if i < len(RANK_EMOJIS) else f"{i+1}."
        name = entry.get("playerName", "Unknown")

        if category == "score":
            value = f"{format_number(entry.get('score', 0))} pts"
        elif category == "kills":
            value = f"{format_number(entry.get('totalKills', 0))} kills"
        elif category == "wave":
            value = f"Wave {entry.get('bestWave', 0)}"
        elif category == "kd":
            kd = entry.get("kdRatio", 0)
            value = f"{kd:.2f} K/D"
        elif category == "bosses":
            value = f"{entry.get('bossKills', 0)} boss"
        elif category == "playtime":
            value = format_duration(entry.get("totalPlaytime", 0))
        else:
            value = "N/A"

        lines.append(f"{emoji} **{name}** | {value}")

    embed.add_field(name="Classement", value="\n".join(lines), inline=False)
    embed.set_footer(text="R-Type Leaderboard")

    return embed


def create_stats_embed(stats: dict, player_name: str) -> discord.Embed:
    """Create player stats embed."""
    embed = discord.Embed(title=f"📊 STATS - {player_name}", color=GREEN)

    # General stats
    kd = stats.get("totalKills", 0) / max(stats.get("totalDeaths", 1), 1)
    general = (
        f"🎯 **Score Total** | {format_number(stats.get('totalScore', 0))}\n"
        f"🏆 **Meilleur Score** | {format_number(stats.get('bestScore', 0))}\n"
        f"⚔️ **Total Kills** | {format_number(stats.get('totalKills', 0))}\n"
        f"💀 **Total Deaths** | {format_number(stats.get('totalDeaths', 0))}\n"
        f"📈 **K/D Ratio** | {kd:.2f}"
    )
    embed.add_field(name="General", value=general, inline=True)

    # Records
    combo = stats.get("bestCombo", 10) / 10
    records = (
        f"🌊 **Meilleure Wave** | {stats.get('bestWave', 0)}\n"
        f"🔥 **Meilleur Combo** | {combo:.1f}x\n"
        f"⏱️ **Temps de jeu** | {format_duration(stats.get('totalPlaytime', 0))}\n"
        f"🎮 **Parties jouees** | {stats.get('gamesPlayed', 0)}\n"
        f"👹 **Boss tues** | {stats.get('bossKills', 0)}"
    )
    embed.add_field(name="Records", value=records, inline=True)

    # Favorite weapon
    weapons = {
        "Standard": stats.get("standardKills", 0),
        "Spread": stats.get("spreadKills", 0),
        "Laser": stats.get("laserKills", 0),
        "Missile": stats.get("missileKills", 0),
        "Wave Cannon": stats.get("waveCannonKills", 0),
    }
    favorite = max(weapons, key=weapons.get)
    emoji = WEAPON_EMOJIS.get(favorite, "🔫")

    embed.add_field(
        name="🔫 Arme favorite",
        value=f"{emoji} **{favorite}** ({format_number(weapons[favorite])} kills)",
        inline=False,
    )

    return embed


def create_kills_embed(stats: dict, player_name: str) -> discord.Embed:
    """Create kills breakdown embed."""
    embed = discord.Embed(title=f"⚔️ KILLS - {player_name}", color=RED)

    weapons = {
        "Standard": stats.get("standardKills", 0),
        "Spread": stats.get("spreadKills", 0),
        "Laser": stats.get("laserKills", 0),
        "Missile": stats.get("missileKills", 0),
        "Wave Cannon": stats.get("waveCannonKills", 0),
    }

    total = sum(weapons.values())
    embed.description = f"**Total: {format_number(total)} kills**\n"

    lines = []
    for name, kills in weapons.items():
        pct = (kills / total * 100) if total > 0 else 0
        bar = progress_bar(kills, total)
        emoji = WEAPON_EMOJIS.get(name, "🔹")
        lines.append(f"{emoji} **{name}** | {bar} | {format_number(kills)} ({pct:.0f}%)")

    embed.add_field(name="Detail par arme", value="\n".join(lines), inline=False)

    return embed


def create_online_embed(sessions: list[dict]) -> discord.Embed:
    """Create online players embed."""
    embed = discord.Embed(
        title=f"🟢 JOUEURS EN LIGNE ({len(sessions)})",
        color=GREEN,
    )

    if not sessions:
        embed.description = "Aucun joueur connecte"
        return embed

    # Group by room
    rooms: dict[str, list[dict]] = {}
    for session in sessions:
        room = session.get("roomCode", "Unknown")
        if room not in rooms:
            rooms[room] = []
        rooms[room].append(session)

    lines = []
    for room, players in rooms.items():
        player_names = ", ".join([p.get("playerName", "???") for p in players])
        wave = players[0].get("currentWave", "?") if players else "?"
        lines.append(f"🎮 **Room {room}** | Wave {wave} | {player_names}")

    embed.add_field(name="Sessions actives", value="\n".join(lines), inline=False)
    embed.set_footer(text=f"Rooms actives: {len(rooms)}")

    return embed


def create_achievements_embed(
    achievements: dict[str, bool], player_name: str
) -> discord.Embed:
    """Create achievements embed."""
    unlocked = sum(1 for v in achievements.values() if v)
    total = len(achievements)

    embed = discord.Embed(
        title=f"🏅 ACHIEVEMENTS - {player_name} ({unlocked}/{total})",
        color=GOLD if unlocked == total else GREEN,
    )

    # Achievement descriptions
    descriptions = {
        "First Blood": "Premier kill",
        "Exterminator": "1000 kills total",
        "Combo Master": "Combo 3.0x atteint",
        "Boss Slayer": "Boss vaincu",
        "Survivor": "Wave 20 sans mourir",
        "Speed Demon": "Wave 10 en <5min",
        "Perfectionist": "Wave sans degats",
        "Veteran": "100 parties jouees",
        "Untouchable": "Partie sans mourir",
        "Weapon Master": "100+ kills chaque arme",
    }

    lines = []
    for name, unlocked in achievements.items():
        status = "✅" if unlocked else "❌"
        desc = descriptions.get(name, "")
        lines.append(f"{status} **{name}** | {desc}")

    embed.add_field(name="Liste", value="\n".join(lines), inline=False)

    return embed


def create_history_embed(history: list[dict], player_name: str) -> discord.Embed:
    """Create game history embed."""
    embed = discord.Embed(title=f"📜 HISTORIQUE - {player_name}", color=BLUE)

    if not history:
        embed.description = "Aucune partie enregistree"
        return embed

    lines = []
    for i, game in enumerate(history, 1):
        when = format_timestamp(game.get("timestamp", 0))
        score = format_number(game.get("score", 0))
        wave = game.get("wave", 0)
        kills = game.get("kills", 0)
        duration = format_game_duration(game.get("duration", 0))

        lines.append(f"{i}. {when} | {score} pts | Wave {wave} | {kills} kills | ⏱️ {duration}")

    embed.add_field(name="Dernieres parties", value="\n".join(lines), inline=False)

    return embed


def create_rank_embed(
    player_name: str,
    all_time: Optional[tuple[int, int]],
    weekly: Optional[tuple[int, int]],
    monthly: Optional[tuple[int, int]],
) -> discord.Embed:
    """Create rank embed."""
    embed = discord.Embed(title=f"📍 RANK - {player_name}", color=GREEN)

    def format_rank(data: Optional[tuple[int, int]]) -> str:
        if data is None:
            return "N/A"
        rank, total = data
        return f"#{rank} / {total} joueurs"

    embed.add_field(name="🌍 All-Time", value=format_rank(all_time), inline=True)
    embed.add_field(name="📅 Weekly", value=format_rank(weekly), inline=True)
    embed.add_field(name="📆 Monthly", value=format_rank(monthly), inline=True)

    return embed
