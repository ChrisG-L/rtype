"""Player stats data class."""

from dataclasses import dataclass, field
from typing import Optional


@dataclass
class PlayerStats:
    """Represents player statistics."""

    player_name: str
    email: str

    # General stats
    games_played: int = 0
    total_kills: int = 0
    total_deaths: int = 0
    total_score: int = 0
    best_score: int = 0
    best_wave: int = 0
    best_combo: int = 10  # x10 format (30 = 3.0x)
    boss_kills: int = 0
    total_playtime: int = 0  # seconds

    # Weapon kills
    standard_kills: int = 0
    spread_kills: int = 0
    laser_kills: int = 0
    missile_kills: int = 0
    wave_cannon_kills: int = 0

    # Achievement bitfield
    achievements: int = 0

    @property
    def kd_ratio(self) -> float:
        """Calculate K/D ratio."""
        if self.total_deaths == 0:
            return float(self.total_kills)
        return self.total_kills / self.total_deaths

    @property
    def combo_display(self) -> str:
        """Format combo for display (30 -> 3.0x)."""
        return f"{self.best_combo / 10:.1f}x"

    @property
    def weapon_kills(self) -> dict[str, int]:
        """Get kills per weapon as dict."""
        return {
            "Standard": self.standard_kills,
            "Spread": self.spread_kills,
            "Laser": self.laser_kills,
            "Missile": self.missile_kills,
            "Wave Cannon": self.wave_cannon_kills,
        }

    @property
    def favorite_weapon(self) -> tuple[str, int]:
        """Get favorite weapon (name, kills)."""
        weapons = self.weapon_kills
        name = max(weapons, key=weapons.get)
        return (name, weapons[name])

    @classmethod
    def from_doc(cls, doc: dict) -> "PlayerStats":
        """Create from MongoDB document."""
        return cls(
            player_name=doc.get("playerName", "Unknown"),
            email=doc.get("email", ""),
            games_played=doc.get("gamesPlayed", 0),
            total_kills=doc.get("totalKills", 0),
            total_deaths=doc.get("totalDeaths", 0),
            total_score=doc.get("totalScore", 0),
            best_score=doc.get("bestScore", 0),
            best_wave=doc.get("bestWave", 0),
            best_combo=doc.get("bestCombo", 10),
            boss_kills=doc.get("bossKills", 0),
            total_playtime=doc.get("totalPlaytime", 0),
            standard_kills=doc.get("standardKills", 0),
            spread_kills=doc.get("spreadKills", 0),
            laser_kills=doc.get("laserKills", 0),
            missile_kills=doc.get("missileKills", 0),
            wave_cannon_kills=doc.get("waveCannonKills", 0),
            achievements=doc.get("achievements", 0),
        )
