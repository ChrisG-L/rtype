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

    Example output:
    ┌────────────────────────────────────────────────────────┐
    │                   REGISTERED USERS                     │
    ├────────────────────────────────────────────────────────┤
    │ Email                        │ Username    │ Status    │
    ├────────────────────────────────────────────────────────┤
    │ test.test@test.test          │ test        │ Offline   │
    │ test2.test2@test2.test2      │ test2       │ Offline   │
    └────────────────────────────────────────────────────────┘
    [CLI] Total: 2 user(s)

    Returns list of user dicts.
    """
    users = []

    for line in output:
        # Skip header, border and empty lines
        if not line or "──" in line or "REGISTERED" in line or "Email" in line:
            continue
        if "[CLI]" in line:
            continue

        # Extract data from table row: │ email │ username │ status │
        # Remove box-drawing characters and split
        clean = line.replace("│", "|").strip()
        if clean.startswith("|"):
            clean = clean[1:]
        if clean.endswith("|"):
            clean = clean[:-1]

        parts = [p.strip() for p in clean.split("|") if p.strip()]

        if len(parts) >= 3:
            users.append({
                "email": parts[0],
                "username": parts[1],
                "status": parts[2]
            })
        elif len(parts) == 2:
            users.append({
                "email": parts[0],
                "username": parts[1],
                "status": "Unknown"
            })

    return users


def parse_sessions_output(output: list[str]) -> list[dict[str, str]]:
    """
    Parse sessions command output.

    Returns list of session dicts.
    """
    sessions = []

    for line in output:
        if not line or "──" in line or "SESSIONS" in line or "Email" in line:
            continue
        if "[CLI]" in line:
            continue
        if "No active sessions" in line:
            return []

        clean = line.replace("│", "|").strip()
        if clean.startswith("|"):
            clean = clean[1:]
        if clean.endswith("|"):
            clean = clean[:-1]

        parts = [p.strip() for p in clean.split("|") if p.strip()]

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

    for line in output:
        if not line or "──" in line or "ROOMS" in line or "Code" in line:
            continue
        if "[CLI]" in line:
            continue
        if "No active rooms" in line:
            return []

        clean = line.replace("│", "|").strip()
        if clean.startswith("|"):
            clean = clean[1:]
        if clean.endswith("|"):
            clean = clean[:-1]

        parts = [p.strip() for p in clean.split("|") if p.strip()]

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
        if not line or "──" in line or "BANNED" in line or "Email" in line:
            continue
        if "[CLI]" in line:
            continue

        clean = line.replace("│", "|").strip()
        if clean.startswith("|"):
            clean = clean[1:]
        if clean.endswith("|"):
            clean = clean[:-1]

        parts = [p.strip() for p in clean.split("|") if p.strip()]

        if len(parts) >= 1:
            bans.append({
                "email": parts[0],
                "reason": parts[1] if len(parts) > 1 else "No reason",
                "date": parts[2] if len(parts) > 2 else "N/A"
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
