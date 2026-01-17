"""Formatting utilities for R-Type Discord Bot."""

from datetime import datetime


def format_number(n: int) -> str:
    """Format number with space separators (1234567 -> 1 234 567)."""
    return f"{n:,}".replace(",", " ")


def format_duration(seconds: int) -> str:
    """Format duration in hours/minutes (3661 -> 1h 1m)."""
    if seconds < 60:
        return f"{seconds}s"
    elif seconds < 3600:
        mins = seconds // 60
        secs = seconds % 60
        return f"{mins}m {secs}s" if secs > 0 else f"{mins}m"
    else:
        hours = seconds // 3600
        minutes = (seconds % 3600) // 60
        return f"{hours}h {minutes}m"


def format_timestamp(ts: int) -> str:
    """Format timestamp as relative time (Il y a 2h, Hier, etc.)."""
    now = datetime.utcnow()
    dt = datetime.utcfromtimestamp(ts)
    diff = now - dt

    if diff.days == 0:
        if diff.seconds < 60:
            return "A l'instant"
        elif diff.seconds < 3600:
            return f"Il y a {diff.seconds // 60}m"
        return f"Il y a {diff.seconds // 3600}h"
    elif diff.days == 1:
        return "Hier"
    elif diff.days < 7:
        return f"Il y a {diff.days}j"
    else:
        return dt.strftime("%d/%m/%Y")


def format_game_duration(seconds: int) -> str:
    """Format game duration as MM:SS."""
    mins = seconds // 60
    secs = seconds % 60
    return f"{mins:02d}:{secs:02d}"


def progress_bar(value: int, max_value: int, length: int = 16) -> str:
    """Create a text progress bar."""
    if max_value == 0:
        return "░" * length
    filled = int((value / max_value) * length)
    return "█" * filled + "░" * (length - filled)
