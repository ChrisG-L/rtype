"""Leaderboard entry data class."""

from dataclasses import dataclass
from typing import Optional


@dataclass
class LeaderboardEntry:
    """Represents a leaderboard entry."""

    rank: int
    player_name: str
    score: int
    wave: int
    kills: int
    duration: int  # seconds
    timestamp: int  # unix timestamp
    email: Optional[str] = None

    @classmethod
    def from_doc(cls, doc: dict, rank: int) -> "LeaderboardEntry":
        """Create from MongoDB document."""
        return cls(
            rank=rank,
            player_name=doc.get("playerName", "Unknown"),
            score=doc.get("score", 0),
            wave=doc.get("wave", 0),
            kills=doc.get("kills", 0),
            duration=doc.get("duration", 0),
            timestamp=doc.get("timestamp", 0),
            email=doc.get("_id") or doc.get("email"),
        )
