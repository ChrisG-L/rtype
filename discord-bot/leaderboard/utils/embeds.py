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

# Mode names and emojis
MODE_NAMES = {0: "Tous", 1: "Solo", 2: "Duo", 3: "Trio", 4: "4P", 5: "5P", 6: "6P"}
MODE_EMOJIS = {1: "👤", 2: "👥", 3: "👥👤", 4: "👥👥", 5: "👥👥👤", 6: "👥👥👥"}


def create_leaderboard_embed(
    entries: list[dict],
    category: str,
    period: str,
    player_count: int = 0,
    limit: int = 10,
) -> discord.Embed:
    """Create leaderboard embed.

    Args:
        entries: List of leaderboard entries
        category: Category type (score, kills, wave, etc.)
        period: Time period (all, daily, weekly, monthly)
        player_count: Game mode filter (0=All, 1=Solo, 2=Duo, etc.)
    """
    period_names = {
        "all": "All-Time",
        "daily": "Daily",
        "weekly": "Weekly",
        "monthly": "Monthly",
    }
    category_names = {
        "score": "Score",
        "kills": "Kills",
        "wave": "Wave",
        "kd": "K/D Ratio",
        "bosses": "Boss Kills",
        "playtime": "Temps de jeu",
    }

    # Build title with mode if filtered
    mode_suffix = f" | {MODE_NAMES.get(player_count, 'Tous')}" if player_count > 0 else ""
    embed = discord.Embed(
        title=f"🏆 LEADERBOARD - TOP {limit} ({period_names.get(period, period)}{mode_suffix})",
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

        # Show mode emoji if showing all modes (player_count=0)
        mode_indicator = ""
        if player_count == 0 and category == "score":
            entry_mode = entry.get("playerCount") or 0
            if entry_mode > 0:
                mode_indicator = f" {MODE_EMOJIS.get(entry_mode, '')}"

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

        lines.append(f"{emoji} **{name}**{mode_indicator} | {value}")

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
        f"💥 **Meilleur Streak** | {stats.get('bestKillStreak', 0)} kills\n"
        f"✨ **Waves Parfaites** | {stats.get('totalPerfectWaves', 0)}\n"
        f"👹 **Boss tues** | {stats.get('bossKills', 0)}"
    )
    embed.add_field(name="Records", value=records, inline=True)

    # Activity
    games_played = stats.get('gamesPlayed', 0)
    total_score = stats.get('totalScore', 0)
    avg_score = total_score // games_played if games_played > 0 else 0
    total_damage = stats.get('totalDamageDealt', 0)

    activity = (
        f"🎮 **Parties jouees** | {games_played}\n"
        f"⏱️ **Temps de jeu** | {format_duration(stats.get('totalPlaytime', 0))}\n"
        f"📊 **Score moyen** | {format_number(avg_score)}\n"
        f"💥 **Degats infliges** | {format_number(total_damage)}"
    )
    embed.add_field(name="Activite", value=activity, inline=False)

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


def _get_rarity_emoji(rarity_pct: float) -> str:
    """Get emoji based on achievement rarity."""
    if rarity_pct < 5:
        return "💎"  # Legendary (<5%)
    elif rarity_pct < 15:
        return "🟣"  # Epic (<15%)
    elif rarity_pct < 30:
        return "🔵"  # Rare (<30%)
    elif rarity_pct < 50:
        return "🟢"  # Uncommon (<50%)
    return "⚪"  # Common (50%+)


def create_achievements_embed(
    achievements: dict[str, int],
    player_name: str,
    rarity: Optional[dict[str, float]] = None,
) -> discord.Embed:
    """Create achievements embed.

    Args:
        achievements: Dict of achievement_name -> unlockedAt timestamp (0 if not unlocked)
        player_name: Player name to display
        rarity: Optional dict of achievement_name -> percentage of players who have it
    """
    unlocked_count = sum(1 for ts in achievements.values() if ts > 0)
    total = len(achievements)

    embed = discord.Embed(
        title=f"🏅 ACHIEVEMENTS - {player_name} ({unlocked_count}/{total})",
        color=GOLD if unlocked_count == total else GREEN,
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
    for name, unlocked_at in achievements.items():
        is_unlocked = unlocked_at > 0
        status = "✅" if is_unlocked else "❌"
        desc = descriptions.get(name, "")

        # Add rarity indicator
        rarity_str = ""
        if rarity and name in rarity:
            pct = rarity[name]
            rarity_emoji = _get_rarity_emoji(pct)
            rarity_str = f" {rarity_emoji} {pct:.0f}%"

        if is_unlocked:
            # Format unlock date
            date_str = format_timestamp(unlocked_at)
            lines.append(f"{status} **{name}** | {desc}{rarity_str} | 🕐 {date_str}")
        else:
            lines.append(f"{status} **{name}** | {desc}{rarity_str}")

    embed.add_field(name="Liste", value="\n".join(lines), inline=False)

    # Add rarity legend
    if rarity:
        embed.set_footer(text="Rarete: 💎<5% 🟣<15% 🔵<30% 🟢<50% ⚪50%+")

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

        # Mode indicator
        game_mode = game.get("playerCount") or 0
        mode_emoji = MODE_EMOJIS.get(game_mode, "") if game_mode > 0 else ""

        # Additional stats
        extras = []
        deaths = game.get("deaths") or 0
        if deaths > 0:
            extras.append(f"💀{deaths}")

        boss_defeated = game.get("bossDefeated") or False
        if boss_defeated:
            extras.append("🏆")

        best_combo = game.get("bestCombo") or 10
        if best_combo > 10:
            combo_val = best_combo / 10
            extras.append(f"🔥{combo_val:.1f}x")

        # Determine main weapon used (highest kills)
        weapon_kills = {
            "Standard": game.get("standardKills") or 0,
            "Spread": game.get("spreadKills") or 0,
            "Laser": game.get("laserKills") or 0,
            "Missile": game.get("missileKills") or 0,
            "Wave Cannon": game.get("waveCannonKills") or 0,
        }
        total_weapon_kills = sum(weapon_kills.values())
        if total_weapon_kills > 0:
            main_weapon = max(weapon_kills, key=weapon_kills.get)
            weapon_emoji = WEAPON_EMOJIS.get(main_weapon, "🔫")
            extras.append(weapon_emoji)

        extra_str = " ".join(extras)
        if extra_str:
            extra_str = f" | {extra_str}"

        lines.append(
            f"{i}. {mode_emoji} {when} | {score} pts | W{wave} | {kills}K | ⏱️{duration}{extra_str}"
        )

    embed.add_field(name="Dernieres parties", value="\n".join(lines), inline=False)
    embed.set_footer(text="👤=Solo 👥=Duo 💀=Deaths 🏆=Boss 🔥=Combo 🔵🟢🔴🟡🟣=Arme")

    return embed


def create_rank_embed(
    player_name: str,
    all_time: Optional[tuple[int, int]],
    weekly: Optional[tuple[int, int]],
    monthly: Optional[tuple[int, int]],
    player_count: int = 0,
) -> discord.Embed:
    """Create rank embed.

    Args:
        player_name: Name of the player
        all_time: (rank, total) for all-time or None
        weekly: (rank, total) for weekly or None
        monthly: (rank, total) for monthly or None
        player_count: Game mode filter (0=All, 1=Solo, 2=Duo, etc.)
    """
    mode_suffix = f" ({MODE_NAMES.get(player_count, 'Tous')})" if player_count > 0 else ""
    embed = discord.Embed(title=f"📍 RANK - {player_name}{mode_suffix}", color=GREEN)

    def format_rank(data: Optional[tuple[int, int]]) -> str:
        if data is None:
            return "N/A"
        rank, total = data
        return f"#{rank} / {total} joueurs"

    embed.add_field(name="🌍 All-Time", value=format_rank(all_time), inline=True)
    embed.add_field(name="📅 Weekly", value=format_rank(weekly), inline=True)
    embed.add_field(name="📆 Monthly", value=format_rank(monthly), inline=True)

    return embed


def create_compare_embed(
    stats1: dict, stats2: dict, name1: str, name2: str
) -> discord.Embed:
    """Create comparison embed for two players."""
    embed = discord.Embed(
        title=f"⚔️ COMPARE",
        description=f"**{name1}** vs **{name2}**",
        color=GOLD,
    )

    def get_indicator(v1: float, v2: float, higher_better: bool) -> str:
        """Return winner indicator."""
        if v1 == v2:
            return "="
        if higher_better:
            return "◀" if v1 > v2 else "▶"
        return "◀" if v1 < v2 else "▶"  # Lower is better

    # Stats to compare
    comparisons = [
        ("Score Total", "totalScore", True),
        ("Meilleur Score", "bestScore", True),
        ("Total Kills", "totalKills", True),
        ("Total Deaths", "totalDeaths", False),
        ("K/D Ratio", None, True),
        ("Meilleure Wave", "bestWave", True),
        ("Meilleur Combo", "bestCombo", True),
        ("Parties", "gamesPlayed", True),
        ("Boss tues", "bossKills", True),
        ("Temps de jeu", "totalPlaytime", True),
    ]

    lines = []
    wins1 = 0
    wins2 = 0

    for label, key, higher_better in comparisons:
        if key == "bestCombo":
            v1 = (stats1.get(key) or 10) / 10
            v2 = (stats2.get(key) or 10) / 10
            fmt1 = f"{v1:.1f}x"
            fmt2 = f"{v2:.1f}x"
        elif key == "totalPlaytime":
            v1 = stats1.get(key) or 0
            v2 = stats2.get(key) or 0
            fmt1 = format_duration(v1)
            fmt2 = format_duration(v2)
        elif key is None:  # K/D Ratio
            kills1 = stats1.get("totalKills") or 0
            deaths1 = max(stats1.get("totalDeaths") or 1, 1)
            kills2 = stats2.get("totalKills") or 0
            deaths2 = max(stats2.get("totalDeaths") or 1, 1)
            v1 = kills1 / deaths1
            v2 = kills2 / deaths2
            fmt1 = f"{v1:.2f}"
            fmt2 = f"{v2:.2f}"
        else:
            v1 = stats1.get(key) or 0
            v2 = stats2.get(key) or 0
            fmt1 = format_number(v1)
            fmt2 = format_number(v2)

        indicator = get_indicator(v1, v2, higher_better)

        # Count wins
        if indicator == "◀":
            wins1 += 1
            lines.append(f"🟢 **{fmt1}** {indicator} {fmt2} | {label}")
        elif indicator == "▶":
            wins2 += 1
            lines.append(f"🔴 {fmt1} {indicator} **{fmt2}** | {label}")
        else:
            lines.append(f"🟡 {fmt1} {indicator} {fmt2} | {label}")

    embed.add_field(name="Comparaison", value="\n".join(lines), inline=False)

    # Winner summary
    if wins1 > wins2:
        winner = f"🏆 **{name1}** gagne {wins1}-{wins2}"
    elif wins2 > wins1:
        winner = f"🏆 **{name2}** gagne {wins2}-{wins1}"
    else:
        winner = f"🤝 **Egalite** {wins1}-{wins2}"

    embed.set_footer(text=winner)

    return embed


def create_server_stats_embed(stats: dict) -> discord.Embed:
    """Create server-wide statistics embed."""
    embed = discord.Embed(
        title="🌐 SERVER STATS - R-Type",
        description="Statistiques globales du serveur",
        color=BLUE,
    )

    # Player stats
    players = (
        f"👥 **Joueurs inscrits** | {format_number(stats.get('totalPlayers', 0))}\n"
        f"🎮 **Parties jouees** | {format_number(stats.get('totalGames', 0))}\n"
        f"⏱️ **Temps de jeu total** | {format_duration(stats.get('totalPlaytime', 0))}"
    )
    embed.add_field(name="Joueurs", value=players, inline=True)

    # Combat stats
    combat = (
        f"⚔️ **Kills totaux** | {format_number(stats.get('totalKills', 0))}\n"
        f"💀 **Deaths totaux** | {format_number(stats.get('totalDeaths', 0))}\n"
        f"👹 **Boss vaincus** | {format_number(stats.get('totalBossKills', 0))}"
    )
    embed.add_field(name="Combat", value=combat, inline=True)

    # Records
    records = (
        f"🏆 **Meilleur score** | {format_number(stats.get('highestScore', 0))}\n"
        f"🌊 **Meilleure wave** | {stats.get('highestWave', 0)}\n"
        f"🔥 **Meilleur combo** | {(stats.get('highestCombo') or 10) / 10:.1f}x"
    )
    embed.add_field(name="Records", value=records, inline=False)

    # Record holders
    if stats.get("scoreHolder"):
        embed.add_field(
            name="🥇 Record Score",
            value=f"**{stats['scoreHolder']}**",
            inline=True,
        )
    if stats.get("waveHolder"):
        embed.add_field(
            name="🥇 Record Wave",
            value=f"**{stats['waveHolder']}**",
            inline=True,
        )

    return embed
