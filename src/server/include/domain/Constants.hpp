/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Constants - All game constants extracted from GameWorld for ECS integration
*/

#ifndef DOMAIN_CONSTANTS_HPP_
#define DOMAIN_CONSTANTS_HPP_

#include <cstdint>

namespace domain::constants {

    // ═══════════════════════════════════════════════════════════════════
    // Screen & World
    // ═══════════════════════════════════════════════════════════════════
    namespace world {
        constexpr float SCREEN_WIDTH = 1920.0f;
        constexpr float SCREEN_HEIGHT = 1080.0f;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Player
    // ═══════════════════════════════════════════════════════════════════
    namespace player {
        constexpr float MOVE_SPEED = 200.0f;
        constexpr float SHIP_WIDTH = 64.0f;
        constexpr float SHIP_HEIGHT = 30.0f;
        constexpr uint8_t DEFAULT_HEALTH = 100;

        // Speed level multipliers (0-3)
        constexpr float SPEED_MULT_LV0 = 1.0f;
        constexpr float SPEED_MULT_LV1 = 1.3f;
        constexpr float SPEED_MULT_LV2 = 1.6f;
        constexpr float SPEED_MULT_LV3 = 1.9f;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Score System
    // ═══════════════════════════════════════════════════════════════════
    namespace score {
        // Points per enemy type
        constexpr uint16_t POINTS_BASIC = 100;
        constexpr uint16_t POINTS_TRACKER = 150;
        constexpr uint16_t POINTS_ZIGZAG = 120;
        constexpr uint16_t POINTS_FAST = 180;
        constexpr uint16_t POINTS_BOMBER = 250;
        constexpr uint16_t POINTS_POW_ARMOR = 200;
        constexpr uint16_t POINTS_BOSS = 5000;
        constexpr uint16_t POINTS_WAVE_BONUS = 500;

        // Combo system
        constexpr float COMBO_GRACE_TIME = 3.0f;    // Grace period before decay
        constexpr float COMBO_DECAY_RATE = 0.5f;    // -0.5x per second after grace
        constexpr float COMBO_INCREMENT = 0.1f;     // +0.1x per kill
        constexpr float COMBO_MAX = 3.0f;           // Max 3.0x multiplier
    }

    // ═══════════════════════════════════════════════════════════════════
    // Weapons (Missiles)
    // ═══════════════════════════════════════════════════════════════════
    namespace weapon {
        // Hitbox
        constexpr float MISSILE_WIDTH = 16.0f;
        constexpr float MISSILE_HEIGHT = 8.0f;
        constexpr float MISSILE_SPAWN_OFFSET_X = 64.0f;
        constexpr float MISSILE_SPAWN_OFFSET_Y = 15.0f;

        // Base speeds (Level 0)
        constexpr float SPEED_STANDARD = 600.0f;
        constexpr float SPEED_SPREAD = 550.0f;
        constexpr float SPEED_LASER = 900.0f;
        constexpr float SPEED_MISSILE = 350.0f;

        // Base damage (Level 0)
        // Balanced for ~60-70 DPS across all weapons
        constexpr uint8_t DAMAGE_STANDARD = 20;   // 20 / 0.3s = 67 dps
        constexpr uint8_t DAMAGE_SPREAD = 8;      // 8×3 / 0.4s = 60 dps
        constexpr uint8_t DAMAGE_LASER = 12;      // 12 / 0.18s = 67 dps
        constexpr uint8_t DAMAGE_MISSILE = 50;    // 50 / 0.7s = 71 dps

        // Base cooldowns (Level 0)
        constexpr float COOLDOWN_STANDARD = 0.3f;
        constexpr float COOLDOWN_SPREAD = 0.4f;
        constexpr float COOLDOWN_LASER = 0.18f;
        constexpr float COOLDOWN_MISSILE = 0.7f;

        // Level multipliers (x100 for integer math)
        // Damage: LV0=100%, LV1=115%, LV2=130%, LV3=150%
        constexpr uint8_t DAMAGE_MULT_LV0 = 100;
        constexpr uint8_t DAMAGE_MULT_LV1 = 115;
        constexpr uint8_t DAMAGE_MULT_LV2 = 130;
        constexpr uint8_t DAMAGE_MULT_LV3 = 150;

        // Cooldown: LV0=100%, LV1=95%, LV2=90%, LV3=85%
        constexpr uint8_t COOLDOWN_MULT_LV0 = 100;
        constexpr uint8_t COOLDOWN_MULT_LV1 = 95;
        constexpr uint8_t COOLDOWN_MULT_LV2 = 90;
        constexpr uint8_t COOLDOWN_MULT_LV3 = 85;

        // Speed: Only LV3 gets 10% bonus
        constexpr uint8_t SPEED_MULT_LV0 = 100;
        constexpr uint8_t SPEED_MULT_LV1 = 100;
        constexpr uint8_t SPEED_MULT_LV2 = 100;
        constexpr uint8_t SPEED_MULT_LV3 = 110;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Enemies
    // ═══════════════════════════════════════════════════════════════════
    namespace enemy {
        // Hitbox
        constexpr float WIDTH = 40.0f;
        constexpr float HEIGHT = 40.0f;

        // Horizontal speed (negative = moving left)
        constexpr float SPEED_X_BASIC = -120.0f;
        constexpr float SPEED_X_TRACKER = -100.0f;
        constexpr float SPEED_X_ZIGZAG = -140.0f;
        constexpr float SPEED_X_FAST = -220.0f;
        constexpr float SPEED_X_BOMBER = -80.0f;
        constexpr float SPEED_X_POW_ARMOR = -90.0f;

        // Movement patterns
        constexpr float AMPLITUDE = 80.0f;
        constexpr float FREQUENCY = 1.8f;
        constexpr float ZIGZAG_SPEED_Y = 300.0f;
        constexpr float ZIGZAG_INTERVAL = 0.8f;
        constexpr float TRACKER_SPEED_Y = 150.0f;

        // Shoot intervals
        constexpr float SHOOT_INTERVAL_BASIC = 2.5f;
        constexpr float SHOOT_INTERVAL_TRACKER = 2.0f;
        constexpr float SHOOT_INTERVAL_ZIGZAG = 3.0f;
        constexpr float SHOOT_INTERVAL_FAST = 1.5f;
        constexpr float SHOOT_INTERVAL_BOMBER = 1.0f;
        constexpr float SHOOT_INTERVAL_POW_ARMOR = 4.0f;

        // Health per type
        constexpr uint8_t HEALTH_BASIC = 40;
        constexpr uint8_t HEALTH_TRACKER = 35;
        constexpr uint8_t HEALTH_ZIGZAG = 30;
        constexpr uint8_t HEALTH_FAST = 25;
        constexpr uint8_t HEALTH_BOMBER = 80;
        constexpr uint8_t HEALTH_POW_ARMOR = 60;

        // Damage dealt
        constexpr uint8_t ENEMY_DAMAGE = 15;
        constexpr uint8_t PLAYER_DAMAGE = 20;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Boss
    // ═══════════════════════════════════════════════════════════════════
    namespace boss {
        constexpr uint16_t SPAWN_WAVE = 10;
        constexpr uint16_t MAX_HEALTH = 1500;
        constexpr uint16_t HP_PER_PLAYER = 1000;  // Multiplayer scaling

        // Phase thresholds (% of max HP)
        constexpr float PHASE2_THRESHOLD = 0.65f;
        constexpr float PHASE3_THRESHOLD = 0.30f;

        // Hitbox
        constexpr float WIDTH = 150.0f;
        constexpr float HEIGHT = 120.0f;

        // Movement
        constexpr float MOVE_SPEED = 150.0f;
        constexpr float DASH_SPEED = 800.0f;

        // Attack timings
        constexpr float CHARGE_TIME = 1.5f;
        constexpr float LASER_WARN_TIME = 1.0f;
        constexpr float LASER_DURATION = 2.0f;
        constexpr float ATTACK_INTERVAL_P1 = 2.0f;
        constexpr float ATTACK_INTERVAL_P2 = 1.5f;
        constexpr float ATTACK_INTERVAL_P3 = 1.0f;

        // Minion spawning
        constexpr uint8_t MAX_MINIONS_P2 = 4;
        constexpr uint8_t MAX_MINIONS_P3 = 8;
        constexpr float MINION_SPAWN_INTERVAL = 5.0f;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Wave Cannon (Charge Shot)
    // ═══════════════════════════════════════════════════════════════════
    namespace wavecannon {
        constexpr float SPEED = 900.0f;
        constexpr float LENGTH = 60.0f;

        // Charge times per level (1-3)
        constexpr float CHARGE_TIME_LV1 = 0.6f;
        constexpr float CHARGE_TIME_LV2 = 1.3f;
        constexpr float CHARGE_TIME_LV3 = 2.2f;

        // Damage per level
        constexpr uint8_t DAMAGE_LV1 = 50;
        constexpr uint8_t DAMAGE_LV2 = 100;
        constexpr uint8_t DAMAGE_LV3 = 250;

        // Beam width per level
        constexpr float WIDTH_LV1 = 20.0f;
        constexpr float WIDTH_LV2 = 35.0f;
        constexpr float WIDTH_LV3 = 55.0f;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Power-ups
    // ═══════════════════════════════════════════════════════════════════
    namespace powerup {
        constexpr float MAX_LIFETIME = 10.0f;
        constexpr float WIDTH = 24.0f;
        constexpr float HEIGHT = 24.0f;
        constexpr float DRIFT_SPEED = -30.0f;

        // Drop rates (percentage)
        constexpr uint8_t DROP_CHANCE = 8;           // 8% on regular enemy
        constexpr uint8_t POW_ARMOR_CHANCE = 50;     // 50% on POW Armor

        // POW Armor spawn interval
        constexpr float POW_ARMOR_SPAWN_INTERVAL = 25.0f;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Force Pod
    // ═══════════════════════════════════════════════════════════════════
    namespace forcepod {
        constexpr float WIDTH = 32.0f;
        constexpr float HEIGHT = 32.0f;
        constexpr float SPEED = 400.0f;
        constexpr float ATTACH_OFFSET_X = 50.0f;
        constexpr uint8_t CONTACT_DAMAGE = 30;
        constexpr float HIT_COOLDOWN = 0.5f;
        constexpr float SHOOT_COOLDOWN = 0.35f;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Bit Devices
    // ═══════════════════════════════════════════════════════════════════
    namespace bitdevice {
        constexpr float WIDTH = 24.0f;
        constexpr float HEIGHT = 24.0f;
        constexpr float ORBIT_RADIUS = 50.0f;
        constexpr float ORBIT_SPEED = 3.0f;  // Radians per second
        constexpr float SHOOT_COOLDOWN = 0.4f;
        constexpr uint8_t CONTACT_DAMAGE = 15;
        constexpr float HIT_COOLDOWN = 0.5f;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Wave Spawning
    // ═══════════════════════════════════════════════════════════════════
    namespace wave {
        constexpr float INTERVAL_MIN = 6.0f;
        constexpr float INTERVAL_MAX = 12.0f;
        constexpr uint8_t ENEMIES_PER_WAVE_MIN = 2;
        constexpr uint8_t ENEMIES_PER_WAVE_MAX = 6;
        constexpr float SPAWN_X = 1950.0f;
        constexpr float SPAWN_Y_MIN = 100.0f;
        constexpr float SPAWN_Y_MAX = 900.0f;
        constexpr float SPAWN_DELAY_MIN = 0.3f;
        constexpr float SPAWN_DELAY_MAX = 1.2f;
    }

}  // namespace domain::constants

#endif /* !DOMAIN_CONSTANTS_HPP_ */
