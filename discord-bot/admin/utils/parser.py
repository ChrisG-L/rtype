"""
Parser for TUI output to extract structured data.
Converts ASCII table output from CLI to Python dictionaries.
"""

import re
from typing import Any


def parse_status_output(output: list[str]) -> dict[str, Any]:
    """
    Parse status command output.

    Example output:
    ┌─────────────────────────────────────────┐
    │              SERVER STATUS              │
    ├─────────────────────────────────────────┤
    │ Active Sessions:        0               │
    │ Players in Game:        0               │
    │ Active Rooms:           0               │
    │ Banned Users:           0               │
    │ Users in DB:            2               │
    ├─────────────────────────────────────────┤
    │ Logs:     ON            Debug: OFF      │
    └─────────────────────────────────────────┘

    Returns dict with parsed values.
    """
    result = {
        "active_sessions": 0,
        "players_in_game": 0,
        "active_rooms": 0,
        "banned_users": 0,
        "users_in_db": 0,
        "logs": "OFF",
        "debug": "OFF"
    }

    text = "\n".join(output)

    # Parse key-value pairs
    patterns = [
        (r"Active Sessions:\s*(\d+)", "active_sessions"),
        (r"Players in Game:\s*(\d+)", "players_in_game"),
        (r"Active Rooms:\s*(\d+)", "active_rooms"),
        (r"Banned Users:\s*(\d+)", "banned_users"),
        (r"Users in DB:\s*(\d+)", "users_in_db"),
        (r"Logs:\s*(ON|OFF)", "logs"),
        (r"Debug:\s*(ON|OFF)", "debug"),
    ]

    for pattern, key in patterns:
        match = re.search(pattern, text)
        if match:
            value = match.group(1)
            if value.isdigit():
                result[key] = int(value)
            else:
                result[key] = value

    return result


def parse_users_output(output: list[str]) -> list[dict[str, str]]:
    """
    Parse users command output.

    Example output (double border):
    ╔═════════════════════════════════════════════════════════════════════════════════╗
    ║                                REGISTERED USERS                                 ║
    ╠═════════════════════════════════════════════════════════════════════════════════╣
    ║ Email                                   Username                 Status         ║
    ╠═════════════════════════════════════════════════════════════════════════════════╣
    ║ test.test@test.test                     test                     Offline        ║
    ╚═════════════════════════════════════════════════════════════════════════════════╝
    [CLI] Total: 2 user(s)

    Returns list of user dicts.
    """
    users = []

    for line in output:
        # Skip header, border and empty lines
        if not line or "═" in line or "─" in line or "REGISTERED" in line:
            continue
        if "[CLI]" in line:
            continue
        # Skip header row with column names
        if "Email" in line and "Username" in line and "Status" in line:
            continue

        # Extract data from table row: ║ email  username  status ║
        # The TUI uses space-separated columns within ║ borders
        clean = line.replace("║", "").replace("│", "").strip()

        if not clean:
            continue

        # Split by multiple spaces (columns are space-padded)
        parts = clean.split()

        if len(parts) >= 3:
            # Last part is status, second-to-last is username, rest is email
            status = parts[-1]
            username = parts[-2]
            email = " ".join(parts[:-2])
            users.append({
                "email": email,
                "username": username,
                "status": status
            })

    return users


def parse_sessions_output(output: list[str]) -> list[dict[str, str]]:
    """
    Parse sessions command output.

    Returns list of session dicts.
    """
    sessions = []
    text = "\n".join(output)

    if "No active sessions" in text:
        return []

    for line in output:
        if not line or "═" in line or "─" in line or "SESSIONS" in line:
            continue
        if "[CLI]" in line:
            continue
        # Skip header row
        if "Email" in line and "Room" in line:
            continue

        clean = line.replace("║", "").replace("│", "").strip()
        if not clean:
            continue

        parts = clean.split()
        if len(parts) >= 2:
            sessions.append({
                "email": parts[0],
                "room": parts[1] if len(parts) > 1 else "N/A",
                "since": parts[2] if len(parts) > 2 else "N/A"
            })

    return sessions


def parse_rooms_output(output: list[str]) -> list[dict[str, str]]:
    """
    Parse rooms command output.

    Returns list of room dicts.
    """
    rooms = []
    text = "\n".join(output)

    if "No active rooms" in text:
        return []

    for line in output:
        if not line or "═" in line or "─" in line or "ROOMS" in line:
            continue
        if "[CLI]" in line:
            continue
        # Skip header row
        if "Code" in line and "Players" in line:
            continue

        clean = line.replace("║", "").replace("│", "").strip()
        if not clean:
            continue

        parts = clean.split()
        if len(parts) >= 2:
            rooms.append({
                "code": parts[0],
                "players": parts[1] if len(parts) > 1 else "0",
                "status": parts[2] if len(parts) > 2 else "Active"
            })

    return rooms


def parse_bans_output(output: list[str]) -> list[dict[str, str]]:
    """
    Parse bans command output.

    Returns list of banned user dicts.
    """
    bans = []

    text = "\n".join(output)
    if "No banned users" in text:
        return []

    for line in output:
        if not line or "═" in line or "─" in line or "BANNED" in line:
            continue
        if "[CLI]" in line:
            continue
        # Skip header row
        if "Email" in line and "Reason" in line:
            continue

        clean = line.replace("║", "").replace("│", "").strip()
        if not clean:
            continue

        parts = clean.split()
        if len(parts) >= 1:
            # First part is email, rest is reason
            email = parts[0]
            reason = " ".join(parts[1:]) if len(parts) > 1 else "No reason"
            bans.append({
                "email": email,
                "reason": reason,
                "date": "N/A"
            })

    return bans


def parse_user_output(output: list[str]) -> dict[str, str]:
    """
    Parse single user details output.

    Returns user dict.
    """
    user = {}
    text = "\n".join(output)

    patterns = [
        (r"Email:\s*(.+?)(?:\s*│|\s*$)", "email"),
        (r"Username:\s*(.+?)(?:\s*│|\s*$)", "username"),
        (r"Status:\s*(\w+)", "status"),
        (r"Created:\s*(.+?)(?:\s*│|\s*$)", "created"),
        (r"Games Played:\s*(\d+)", "games_played"),
        (r"Best Score:\s*(\d+)", "best_score"),
        (r"Total Kills:\s*(\d+)", "total_kills"),
    ]

    for pattern, key in patterns:
        match = re.search(pattern, text)
        if match:
            user[key] = match.group(1).strip()

    return user


def clean_cli_prefix(output: list[str]) -> list[str]:
    """Remove [CLI] prefixes and empty lines from output."""
    cleaned = []
    for line in output:
        if line.startswith("[CLI] "):
            line = line[6:]
        if line.strip():
            cleaned.append(line)
    return cleaned
