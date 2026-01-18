/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** DomainBridge - Implementation
*/

#include "infrastructure/ecs/bridge/DomainBridge.hpp"

namespace infrastructure::ecs::bridge {

    DomainBridge::DomainBridge(
        domain::services::GameRule& gameRule,
        domain::services::CollisionRule& collisionRule,
        domain::services::EnemyBehavior& enemyBehavior
    ) : _gameRule(gameRule),
        _collisionRule(collisionRule),
        _enemyBehavior(enemyBehavior)
    {}

    // ═══════════════════════════════════════════════════════════════════
    // Damage & Health
    // ═══════════════════════════════════════════════════════════════════

    DamageResult DomainBridge::applyDamage(components::HealthComp& health, uint16_t rawDamage) {
        DamageResult result;

        // Check invulnerability (GodMode)
        if (health.invulnerable) {
            result.actualDamage = 0;
            result.died = false;
            return result;
        }

        // Apply damage
        result.actualDamage = rawDamage;
        result.died = _gameRule.shouldEntityDie(health.current, rawDamage);
        health.current = _gameRule.applyDamage(health.current, rawDamage);

        return result;
    }

    bool DomainBridge::wouldDie(uint16_t currentHP, uint16_t damage) const {
        return _gameRule.shouldEntityDie(currentHP, damage);
    }

    // ═══════════════════════════════════════════════════════════════════
    // Score & Combo
    // ═══════════════════════════════════════════════════════════════════

    uint32_t DomainBridge::calculateKillScore(uint16_t basePoints, float comboMultiplier) const {
        return _gameRule.applyComboBonus(basePoints, comboMultiplier);
    }

    uint16_t DomainBridge::getEnemyPoints(uint8_t enemyType) const {
        return _gameRule.getEnemyPointValue(enemyType);
    }

    float DomainBridge::incrementCombo(float currentCombo) const {
        return _gameRule.incrementCombo(currentCombo);
    }

    float DomainBridge::decayCombo(float currentCombo, float deltaTime, float timeSinceKill) const {
        return _gameRule.decayCombo(currentCombo, deltaTime, timeSinceKill);
    }

    float DomainBridge::getComboGraceTime() const {
        return _gameRule.getComboGraceTime();
    }

    // ═══════════════════════════════════════════════════════════════════
    // Weapon Stats
    // ═══════════════════════════════════════════════════════════════════

    uint8_t DomainBridge::getMissileDamage(uint8_t weaponType, uint8_t level) const {
        return _gameRule.getMissileDamage(weaponType, level);
    }

    float DomainBridge::getMissileSpeed(uint8_t weaponType, uint8_t level) const {
        return _gameRule.getMissileSpeed(weaponType, level);
    }

    float DomainBridge::getWeaponCooldown(uint8_t weaponType, uint8_t level) const {
        return _gameRule.getWeaponCooldown(weaponType, level);
    }

    // ═══════════════════════════════════════════════════════════════════
    // Player Movement
    // ═══════════════════════════════════════════════════════════════════

    float DomainBridge::getPlayerSpeed(uint8_t speedLevel) const {
        float baseSpeed = _gameRule.getPlayerBaseSpeed();
        float multiplier = _gameRule.getSpeedMultiplier(speedLevel);
        return baseSpeed * multiplier;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Collision Detection
    // ═══════════════════════════════════════════════════════════════════

    bool DomainBridge::checkCollision(float x1, float y1, float w1, float h1,
                                      float x2, float y2, float w2, float h2) const {
        domain::services::HitboxData a{x1, y1, w1, h1};
        domain::services::HitboxData b{x2, y2, w2, h2};
        return _collisionRule.checkAABB(a, b);
    }

    bool DomainBridge::isOutOfBounds(float x, float y, float w, float h) const {
        domain::services::HitboxData hitbox{x, y, w, h};
        return _collisionRule.isOutOfBounds(hitbox);
    }

    void DomainBridge::clampToScreen(float& x, float& y, float w, float h) const {
        _collisionRule.clampToScreen(x, y, w, h);
    }

    // ═══════════════════════════════════════════════════════════════════
    // Enemy Behavior
    // ═══════════════════════════════════════════════════════════════════

    std::pair<float, float> DomainBridge::getEnemyMovement(
        uint8_t enemyType,
        float currentY,
        float baseY,
        float aliveTime,
        float phaseOffset,
        float deltaTime,
        float targetY,
        domain::services::ZigzagState& zigzag
    ) {
        domain::services::EnemyMovementInput input;
        input.enemyType = enemyType;
        input.currentX = 0.0f;  // X handled separately via getHorizontalMovement
        input.currentY = currentY;
        input.baseY = baseY;
        input.aliveTime = aliveTime;
        input.phaseOffset = phaseOffset;
        input.deltaTime = deltaTime;
        input.zigzag = zigzag;
        input.targetY = targetY;

        auto output = _enemyBehavior.calculateMovement(input);

        // Update zigzag state for caller
        zigzag = output.zigzag;

        // Return X and Y movements
        float dx = _enemyBehavior.getHorizontalMovement(enemyType, deltaTime);
        float dy = output.newY - currentY;

        return {dx, dy};
    }

    bool DomainBridge::canEnemyShoot(float shootCooldown) const {
        return _enemyBehavior.canShoot(shootCooldown);
    }

    float DomainBridge::getEnemyShootInterval(uint8_t enemyType) const {
        return _gameRule.getEnemyShootInterval(enemyType);
    }

}  // namespace infrastructure::ecs::bridge
