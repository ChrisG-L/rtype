/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** DomainBridge - Bridge between ECS Infrastructure and Domain Services
*/

#ifndef DOMAIN_BRIDGE_HPP_
#define DOMAIN_BRIDGE_HPP_

#include "domain/services/GameRule.hpp"
#include "domain/services/CollisionRule.hpp"
#include "domain/services/EnemyBehavior.hpp"
#include "../components/HealthComp.hpp"
#include <cstdint>
#include <utility>

namespace infrastructure::ecs::bridge {

    /**
     * @brief Result of applying damage to an entity.
     */
    struct DamageResult {
        uint16_t actualDamage;  // Damage actually applied (0 if invulnerable)
        bool died;              // Entity HP reached 0
    };

    /**
     * @brief Bridge between ECS Systems and Domain Services.
     *
     * This class implements the Hexagonal Architecture pattern:
     * - ECS Systems (Infrastructure) delegate business logic to Domain
     * - Domain Services remain unaware of ECS implementation
     * - All game rules flow through this bridge
     *
     * Usage:
     *   DomainBridge bridge(gameRule, collisionRule, enemyBehavior);
     *   auto result = bridge.applyDamage(healthComp, 30);
     *   if (result.died) { ... }
     */
    class DomainBridge {
    public:
        DomainBridge(
            domain::services::GameRule& gameRule,
            domain::services::CollisionRule& collisionRule,
            domain::services::EnemyBehavior& enemyBehavior
        );

        ~DomainBridge() = default;

        // ═══════════════════════════════════════════════════════════════════
        // Damage & Health (delegates to GameRule)
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Apply damage to a HealthComp.
         * @param health HealthComp to modify
         * @param rawDamage Incoming damage
         * @return DamageResult with actual damage and death flag
         */
        DamageResult applyDamage(components::HealthComp& health, uint16_t rawDamage);

        /**
         * @brief Check if damage would be lethal.
         * @param currentHP Current health
         * @param damage Incoming damage
         * @return true if would die
         */
        bool wouldDie(uint16_t currentHP, uint16_t damage) const;

        // ═══════════════════════════════════════════════════════════════════
        // Score & Combo (delegates to GameRule)
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Calculate score for a kill.
         * @param basePoints Base point value (from enemy type)
         * @param comboMultiplier Current combo multiplier
         * @return Final score with combo applied
         */
        uint32_t calculateKillScore(uint16_t basePoints, float comboMultiplier) const;

        /**
         * @brief Get base points for an enemy type.
         * @param enemyType Enemy type ID
         * @return Base point value
         */
        uint16_t getEnemyPoints(uint8_t enemyType) const;

        /**
         * @brief Increment combo after a kill.
         * @param currentCombo Current combo multiplier
         * @return New combo multiplier
         */
        float incrementCombo(float currentCombo) const;

        /**
         * @brief Decay combo over time.
         * @param currentCombo Current combo
         * @param deltaTime Time since last update
         * @param timeSinceKill Time since last kill
         * @return Decayed combo
         */
        float decayCombo(float currentCombo, float deltaTime, float timeSinceKill) const;

        /**
         * @brief Get combo grace time before decay.
         * @return Grace time in seconds
         */
        float getComboGraceTime() const;

        // ═══════════════════════════════════════════════════════════════════
        // Weapon Stats (delegates to GameRule)
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Get missile damage for weapon type and level.
         */
        uint8_t getMissileDamage(uint8_t weaponType, uint8_t level) const;

        /**
         * @brief Get missile speed for weapon type and level.
         */
        float getMissileSpeed(uint8_t weaponType, uint8_t level) const;

        /**
         * @brief Get weapon cooldown for weapon type and level.
         */
        float getWeaponCooldown(uint8_t weaponType, uint8_t level) const;

        // ═══════════════════════════════════════════════════════════════════
        // Player Movement (delegates to GameRule)
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Get player speed with level multiplier.
         * @param speedLevel Speed upgrade level (0-3)
         * @return Speed in pixels per second
         */
        float getPlayerSpeed(uint8_t speedLevel) const;

        // ═══════════════════════════════════════════════════════════════════
        // Collision Detection (delegates to CollisionRule)
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Check AABB collision between two hitboxes.
         */
        bool checkCollision(float x1, float y1, float w1, float h1,
                           float x2, float y2, float w2, float h2) const;

        /**
         * @brief Check if entity is fully outside screen bounds.
         */
        bool isOutOfBounds(float x, float y, float w, float h) const;

        /**
         * @brief Clamp position to screen bounds.
         */
        void clampToScreen(float& x, float& y, float w, float h) const;

        // ═══════════════════════════════════════════════════════════════════
        // Enemy Behavior (delegates to EnemyBehavior)
        // ═══════════════════════════════════════════════════════════════════

        /**
         * @brief Get enemy movement delta.
         * @return pair<dx, dy>
         */
        std::pair<float, float> getEnemyMovement(
            uint8_t enemyType,
            float currentY,
            float baseY,
            float aliveTime,
            float phaseOffset,
            float deltaTime,
            float targetY,
            domain::services::ZigzagState& zigzag
        );

        /**
         * @brief Check if enemy can shoot.
         */
        bool canEnemyShoot(float shootCooldown) const;

        /**
         * @brief Get enemy shoot interval.
         */
        float getEnemyShootInterval(uint8_t enemyType) const;

    private:
        domain::services::GameRule& _gameRule;
        domain::services::CollisionRule& _collisionRule;
        domain::services::EnemyBehavior& _enemyBehavior;
    };

}  // namespace infrastructure::ecs::bridge

#endif /* !DOMAIN_BRIDGE_HPP_ */
