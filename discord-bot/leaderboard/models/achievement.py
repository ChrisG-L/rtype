"""Achievement data class and definitions."""

from dataclasses import dataclass


@dataclass
class Achievement:
    """Represents an achievement."""

    name: str
    description: str
    bit: int
    unlocked: bool = False


# Achievement definitions matching R-Type server
ACHIEVEMENT_DEFINITIONS = [
    Achievement("First Blood", "Premier kill", 0),
    Achievement("Exterminator", "1000 kills total", 1),
    Achievement("Combo Master", "Combo 3.0x atteint", 2),
    Achievement("Boss Slayer", "Boss vaincu", 3),
    Achievement("Survivor", "Wave 20 sans mourir", 4),
    Achievement("Speed Demon", "Wave 10 en <5min", 5),
    Achievement("Perfectionist", "Wave sans degats", 6),
    Achievement("Veteran", "100 parties jouees", 7),
    Achievement("Untouchable", "Partie sans mourir", 8),
    Achievement("Weapon Master", "100+ kills chaque arme", 9),
]


def get_achievements_from_bitfield(bitfield: int) -> list[Achievement]:
    """Convert achievement bitfield to list of Achievement objects."""
    achievements = []
    for ach in ACHIEVEMENT_DEFINITIONS:
        unlocked = bool(bitfield & (1 << ach.bit))
        achievements.append(
            Achievement(
                name=ach.name,
                description=ach.description,
                bit=ach.bit,
                unlocked=unlocked,
            )
        )
    return achievements
