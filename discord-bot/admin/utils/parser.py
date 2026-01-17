"""
Parser for TUI output to extract structured data.
Converts ASCII table output from CLI to Python dictionaries.
"""

import re
from typing import Any


def parse_status_output(output: list[str]) -> dict[str, Any]:
    """
    Parse status command output.

    TUI format (single border):
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

    TUI format (double border, fixed-width columns):
    ╔═══════════════════════════════════════════════════════════════════════════════════╗
    ║                                REGISTERED USERS                                   ║
    ╠═══════════════════════════════════════════════════════════════════════════════════╣
    ║ Email                                   Username                 Status           ║
    ╠═══════════════════════════════════════════════════════════════════════════════════╣
    ║ test.test@test.test                     test                     Offline          ║
    ╚═══════════════════════════════════════════════════════════════════════════════════╝
    [CLI] Total: 2 user(s)

    Column widths: Email=40, Username=25, Status=14

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

        # Remove border characters
        clean = line.replace("║", "").replace("│", "").strip()

        if not clean:
            continue

        # Fixed-width parsing: Email(40) + Username(25) + Status(14)
        # Use split but verify we have data rows
        parts = clean.split()

        if len(parts) >= 3:
            # Last part is status (Online/Offline/Banned), second-to-last is username
            status = parts[-1]
            if status not in ("Online", "Offline", "Banned"):
                continue
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

    TUI format (double border, fixed-width columns):
    ╔═══════════════════════════════════════════════════════════════════════════════════════════════╗
    ║                                        ACTIVE SESSIONS                                        ║
    ╠═══════════════════════════════════════════════════════════════════════════════════════════════╣
    ║ Email                    Display Name   Status    Room     Player ID Endpoint                 ║
    ╠═══════════════════════════════════════════════════════════════════════════════════════════════╣
    ║ test@test.com            test           Active    ABC123   0         127.0.0.1:12345          ║
    ╚═══════════════════════════════════════════════════════════════════════════════════════════════╝

    Column widths: Email=25, Display Name=15, Status=10, Room=9, Player ID=10, Endpoint=22

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
        if "Email" in line and "Display Name" in line:
            continue

        clean = line.replace("║", "").replace("│", "").strip()
        if not clean:
            continue

        parts = clean.split()
        # Need at least: Email DisplayName Status Room
        if len(parts) >= 4:
            email = parts[0]
            # Find Status field (Active/Pending/Expired)
            status_idx = -1
            for i, p in enumerate(parts):
                if p in ("Active", "Pending", "Expired"):
                    status_idx = i
                    break

            if status_idx > 0:
                display_name = " ".join(parts[1:status_idx])
                status = parts[status_idx]
                room = parts[status_idx + 1] if len(parts) > status_idx + 1 else "-"
                player_id = parts[status_idx + 2] if len(parts) > status_idx + 2 else "-"
                endpoint = parts[status_idx + 3] if len(parts) > status_idx + 3 else ""

                sessions.append({
                    "email": email,
                    "display_name": display_name,
                    "status": status,
                    "room": room if room != "-" else "N/A",
                    "player_id": player_id,
                    "endpoint": endpoint
                })

    return sessions


def parse_rooms_output(output: list[str]) -> list[dict[str, str]]:
    """
    Parse rooms command output.

    TUI format (double border, fixed-width columns):
    ╔══════════════════════════════════════════════════════════════════════════════════╗
    ║                                   ACTIVE ROOMS                                   ║
    ╠══════════════════════════════════════════════════════════════════════════════════╣
    ║ Code     Name                     Players   State       Private   Host           ║
    ╠══════════════════════════════════════════════════════════════════════════════════╣
    ║ G3DPML   gfred                    2/4       InGame      No        test@test.com  ║
    ╚══════════════════════════════════════════════════════════════════════════════════╝

    Column widths: Code=8, Name=25, Players=10, State=12, Private=10, Host=15

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
        if "Code" in line and "Players" in line and "State" in line:
            continue

        clean = line.replace("║", "").replace("│", "").strip()
        if not clean:
            continue

        parts = clean.split()
        # At least: Code Name Players State
        if len(parts) >= 4:
            code = parts[0]
            # Find the players field (contains / like "2/4")
            players_idx = -1
            for i, p in enumerate(parts):
                if "/" in p and all(c.isdigit() or c == "/" for c in p):
                    players_idx = i
                    break

            if players_idx > 0:
                name = " ".join(parts[1:players_idx])
                players = parts[players_idx]
                state = parts[players_idx + 1] if len(parts) > players_idx + 1 else "Unknown"
                private = parts[players_idx + 2] if len(parts) > players_idx + 2 else "No"
                host = parts[-1] if len(parts) > players_idx + 3 else ""

                rooms.append({
                    "code": code,
                    "name": name,
                    "players": players,
                    "status": state,
                    "private": private,
                    "host": host
                })

    return rooms


def parse_bans_output(output: list[str]) -> list[dict[str, str]]:
    """
    Parse bans command output.

    TUI format (double border, fixed-width columns):
    ╔═════════════════════════════════════════════════════════════════════════════════╗
    ║                                  BANNED USERS                                   ║
    ╠═════════════════════════════════════════════════════════════════════════════════╣
    ║ Email                                   Display Name                            ║
    ╠═════════════════════════════════════════════════════════════════════════════════╣
    ║ test@test.com                           Test User                               ║
    ╚═════════════════════════════════════════════════════════════════════════════════╝

    Column widths: Email=40, Display Name=38

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
        if "Email" in line and "Display Name" in line:
            continue

        clean = line.replace("║", "").replace("│", "").strip()
        if not clean:
            continue

        parts = clean.split()
        if len(parts) >= 1:
            # First part is email, rest is display name (can be "-" if empty)
            email = parts[0]
            display_name = " ".join(parts[1:]) if len(parts) > 1 else "-"
            bans.append({
                "email": email,
                "display_name": display_name
            })

    return bans


def parse_user_output(output: list[str]) -> dict[str, str]:
    """
    Parse single user details output.

    TUI format (double border):
    ╔═════════════════════════════════════════════════════════════════╗
    ║                         USER DETAILS                            ║
    ╠═════════════════════════════════════════════════════════════════╣
    ║ Email:      test@test.com                                       ║
    ║ Username:   testuser                                            ║
    ║ Password:   $2b$12$...                                          ║
    ║ User ID:    abc123                                              ║
    ║ Status:     Online                                              ║
    ╠═════════════════════════════════════════════════════════════════╣
    ║ Created:    2024-01-15 10:30:00                                 ║
    ║ Last Login: 2024-01-16 09:15:00                                 ║
    ╚═════════════════════════════════════════════════════════════════╝

    Returns user dict with all fields.
    """
    user = {}
    text = "\n".join(output)

    patterns = [
        (r"Email:\s*(.+?)(?:\s*║|\s*$)", "email"),
        (r"Username:\s*(.+?)(?:\s*║|\s*$)", "username"),
        (r"User ID:\s*(.+?)(?:\s*║|\s*$)", "user_id"),
        (r"Status:\s*(\w+)", "status"),
        (r"Created:\s*(.+?)(?:\s*║|\s*$)", "created"),
        (r"Last Login:\s*(.+?)(?:\s*║|\s*$)", "last_login"),
        (r"Games Played:\s*(\d+)", "games_played"),
        (r"Best Score:\s*(\d+)", "best_score"),
        (r"Total Kills:\s*(\d+)", "total_kills"),
    ]

    for pattern, key in patterns:
        match = re.search(pattern, text)
        if match:
            user[key] = match.group(1).strip()

    return user


def parse_room_details_output(output: list[str]) -> dict[str, Any]:
    """
    Parse single room details output.

    TUI format (double border):
    ╔═════════════════════════════════════════════════════════════════╗
    ║                         ROOM DETAILS                            ║
    ╠═════════════════════════════════════════════════════════════════╣
    ║ Code:       ABC123                                              ║
    ║ Name:       My Room                                             ║
    ║ Host:       test@test.com                                       ║
    ║ Players:    2/4                                                 ║
    ║ State:      Waiting                                             ║
    ║ Private:    No                                                  ║
    ╠═════════════════════════════════════════════════════════════════╣
    ║                             PLAYERS                             ║
    ╠═════════════════════════════════════════════════════════════════╣
    ║ Slot  Display Name        Ready     Host      Email             ║
    ╠═════════════════════════════════════════════════════════════════╣
    ║ 0     test                Yes       Yes       test@test.com     ║
    ║ 1     player2             Yes       No        player2@test.com  ║
    ╚═════════════════════════════════════════════════════════════════╝

    Column widths for players: Slot=6, Display Name=20, Ready=10, Host=10, Email=18

    Returns room dict with players list.
    """
    room: dict[str, Any] = {
        "code": "",
        "name": "",
        "host": "",
        "players": "",
        "state": "",
        "private": "",
        "player_list": []
    }

    text = "\n".join(output)

    # Parse room details
    patterns = [
        (r"Code:\s*(\S+)", "code"),
        (r"Name:\s*(.+?)(?:\s*║|\s*$)", "name"),
        (r"Host:\s*(\S+)", "host"),
        (r"Players:\s*(\d+/\d+)", "players"),
        (r"State:\s*(\w+)", "state"),
        (r"Private:\s*(\w+)", "private"),
    ]

    for pattern, key in patterns:
        match = re.search(pattern, text)
        if match:
            room[key] = match.group(1).strip()

    # Parse players table
    in_players_section = False
    for line in output:
        if "PLAYERS" in line and "║" in line:
            in_players_section = True
            continue

        if not in_players_section:
            continue

        # Skip borders, header row, and options section
        if not line or "═" in line or "─" in line or "OPTIONS" in line:
            continue
        if "Slot" in line and "Display Name" in line:
            continue

        clean = line.replace("║", "").replace("│", "").strip()
        if not clean:
            continue

        parts = clean.split()
        # Format: Slot DisplayName Ready Host Email
        # Slot must be a digit (0, 1, 2, 3)
        if len(parts) >= 4 and parts[0].isdigit():
            slot = parts[0]
            # Find Ready field (Yes/No)
            ready_idx = -1
            for i, p in enumerate(parts):
                if p in ("Yes", "No"):
                    ready_idx = i
                    break

            if ready_idx > 1:
                display_name = " ".join(parts[1:ready_idx])
                ready = parts[ready_idx]
                is_host = parts[ready_idx + 1] if len(parts) > ready_idx + 1 and parts[ready_idx + 1] in ("Yes", "No") else "No"
                # Email is after Host field
                email_idx = ready_idx + 2 if is_host in ("Yes", "No") else ready_idx + 1
                email = parts[email_idx] if len(parts) > email_idx else ""

                room["player_list"].append({
                    "slot": slot,
                    "display_name": display_name,
                    "ready": ready,
                    "is_host": is_host,
                    "email": email
                })

    return room


def clean_cli_prefix(output: list[str]) -> list[str]:
    """Remove [CLI] prefixes and empty lines from output."""
    cleaned = []
    for line in output:
        if line.startswith("[CLI] "):
            line = line[6:]
        if line.strip():
            cleaned.append(line)
    return cleaned
