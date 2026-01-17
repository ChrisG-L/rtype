"""Game history entry data class."""

from dataclasses import dataclass


@dataclass
class GameHistoryEntry:
    """Represents a single game session in history."""

    score: int
    wave: int
    kills: int
    deaths: int
    duration: int  # seconds
    timestamp: int  # unix timestamp

    @classmethod
    def from_doc(cls, doc: dict) -> "GameHistoryEntry":
        """Create from MongoDB document."""
        return cls(
            score=doc.get("score", 0),
            wave=doc.get("wave", 0),
            kills=doc.get("kills", 0),
            deaths=doc.get("deaths", 0),
            duration=doc.get("duration", 0),
            timestamp=doc.get("timestamp", 0),
        )
