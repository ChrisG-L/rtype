/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** DamageSystem Tests - Verifies damage application from collisions
*/

#include <gtest/gtest.h>
#include "infrastructure/ecs/core/ECS.hpp"
#include "infrastructure/ecs/components/PositionComp.hpp"
#include "infrastructure/ecs/components/HitboxComp.hpp"
#include "infrastructure/ecs/components/HealthComp.hpp"
#include "infrastructure/ecs/components/MissileTag.hpp"
#include "infrastructure/ecs/components/EnemyTag.hpp"
#include "infrastructure/ecs/components/OwnerComp.hpp"
#include "infrastructure/ecs/systems/CollisionSystem.hpp"
#include "infrastructure/ecs/systems/DamageSystem.hpp"
#include "infrastructure/ecs/bridge/DomainBridge.hpp"
#include "domain/services/GameRule.hpp"
#include "domain/services/CollisionRule.hpp"
#include "domain/services/EnemyBehavior.hpp"

using namespace infrastructure::ecs::components;
using namespace infrastructure::ecs::systems;
using namespace infrastructure::ecs::bridge;

// ═══════════════════════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════════════════════

class DamageSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        _ecs.registerComponent<PositionComp>();
        _ecs.registerComponent<HitboxComp>();
        _ecs.registerComponent<HealthComp>();
        _ecs.registerComponent<MissileTag>();
        _ecs.registerComponent<EnemyTag>();
        _ecs.registerComponent<OwnerComp>();
    }

    // Helper to create overlapping entities
    void makeOverlapping(ECS::EntityID e1, ECS::EntityID e2, float x = 100.0f, float y = 100.0f) {
        auto& pos1 = _ecs.entityAddComponent<PositionComp>(e1);
        pos1.x = x;
        pos1.y = y;
        auto& hitbox1 = _ecs.entityAddComponent<HitboxComp>(e1);
        hitbox1.width = 50.0f;
        hitbox1.height = 50.0f;

        auto& pos2 = _ecs.entityAddComponent<PositionComp>(e2);
        pos2.x = x;
        pos2.y = y;
        auto& hitbox2 = _ecs.entityAddComponent<HitboxComp>(e2);
        hitbox2.width = 50.0f;
        hitbox2.height = 50.0f;
    }

    ECS::ECS _ecs;

    // Domain services
    domain::services::GameRule _gameRule;
    domain::services::CollisionRule _collisionRule;
    domain::services::EnemyBehavior _enemyBehavior;
};

// ═══════════════════════════════════════════════════════════════════════════
// Missile vs Enemy Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(DamageSystemTest, MissileHitsEnemyAppliesDamage) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem collisionSystem(bridge);
    DamageSystem damageSystem(bridge, collisionSystem);

    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto enemy = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    makeOverlapping(missile, enemy);

    auto& missileTag = _ecs.entityAddComponent<MissileTag>(missile);
    missileTag.baseDamage = 20;

    auto& enemyHealth = _ecs.entityAddComponent<HealthComp>(enemy);
    enemyHealth.current = 40;
    enemyHealth.max = 40;

    auto& enemyTag = _ecs.entityAddComponent<EnemyTag>(enemy);
    enemyTag.type = 0;
    enemyTag.points = 100;

    // Detect collision
    collisionSystem.Update(_ecs, 0, 0);
    ASSERT_EQ(collisionSystem.getCollisions().size(), 1);

    // Apply damage
    damageSystem.Update(_ecs, 0, 0);

    // Enemy should have taken damage
    EXPECT_EQ(enemyHealth.current, 20);

    // Missile should be deleted (consumed)
    EXPECT_FALSE(_ecs.entityIsActive(missile));
}

TEST_F(DamageSystemTest, MissileKillsEnemy) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem collisionSystem(bridge);
    DamageSystem damageSystem(bridge, collisionSystem);

    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto enemy = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    makeOverlapping(missile, enemy);

    auto& missileTag = _ecs.entityAddComponent<MissileTag>(missile);
    missileTag.baseDamage = 50;  // Enough to kill

    auto& enemyHealth = _ecs.entityAddComponent<HealthComp>(enemy);
    enemyHealth.current = 40;
    enemyHealth.max = 40;

    auto& enemyTag = _ecs.entityAddComponent<EnemyTag>(enemy);
    enemyTag.type = 0;
    enemyTag.points = 100;

    collisionSystem.Update(_ecs, 0, 0);
    damageSystem.Update(_ecs, 0, 0);

    // Both should be deleted
    EXPECT_FALSE(_ecs.entityIsActive(missile));
    EXPECT_FALSE(_ecs.entityIsActive(enemy));

    // Kill event should be recorded
    const auto& kills = damageSystem.getKillEvents();
    ASSERT_EQ(kills.size(), 1);
    EXPECT_EQ(kills[0].basePoints, 100);
    EXPECT_EQ(kills[0].killedType, 0);
}

TEST_F(DamageSystemTest, KillEventContainsOwnerPlayerId) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem collisionSystem(bridge);
    DamageSystem damageSystem(bridge, collisionSystem);

    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto enemy = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    makeOverlapping(missile, enemy);

    auto& missileTag = _ecs.entityAddComponent<MissileTag>(missile);
    missileTag.baseDamage = 50;

    auto& ownerComp = _ecs.entityAddComponent<OwnerComp>(missile);
    ownerComp.ownerId = 42;  // Player 42 fired this missile
    ownerComp.isPlayerOwned = true;

    auto& enemyHealth = _ecs.entityAddComponent<HealthComp>(enemy);
    enemyHealth.current = 40;
    enemyHealth.max = 40;

    auto& enemyTag = _ecs.entityAddComponent<EnemyTag>(enemy);
    enemyTag.type = 1;
    enemyTag.points = 150;

    collisionSystem.Update(_ecs, 0, 0);
    damageSystem.Update(_ecs, 0, 0);

    const auto& kills = damageSystem.getKillEvents();
    ASSERT_EQ(kills.size(), 1);
    EXPECT_EQ(kills[0].killerPlayerId, 42);
}

// ═══════════════════════════════════════════════════════════════════════════
// Player Invulnerability Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(DamageSystemTest, GodModePreventsDamage) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem collisionSystem(bridge);
    DamageSystem damageSystem(bridge, collisionSystem);

    auto player = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto enemy = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

    makeOverlapping(player, enemy);

    auto& playerHealth = _ecs.entityAddComponent<HealthComp>(player);
    playerHealth.current = 100;
    playerHealth.max = 100;
    playerHealth.invulnerable = true;  // GodMode ON

    _ecs.entityAddComponent<HealthComp>(enemy);

    collisionSystem.Update(_ecs, 0, 0);
    damageSystem.Update(_ecs, 0, 0);

    // Player should not have taken damage
    EXPECT_EQ(playerHealth.current, 100);
}

// NOTE: PlayerTakesContactDamageFromEnemy test removed
// R-Type design: NO player-enemy contact damage (confirmed in legacy code)
// Damage only comes from: enemy missiles, boss attacks

// ═══════════════════════════════════════════════════════════════════════════
// Friendly Fire Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(DamageSystemTest, FriendlyFireDisabled) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem collisionSystem(bridge);
    DamageSystem damageSystem(bridge, collisionSystem);

    auto player = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

    makeOverlapping(player, missile);

    auto& playerHealth = _ecs.entityAddComponent<HealthComp>(player);
    playerHealth.current = 100;
    playerHealth.max = 100;

    auto& missileTag = _ecs.entityAddComponent<MissileTag>(missile);
    missileTag.baseDamage = 20;

    collisionSystem.Update(_ecs, 0, 0);
    damageSystem.Update(_ecs, 0, 0);

    // Player should NOT take damage from friendly missile
    EXPECT_EQ(playerHealth.current, 100);

    // Missile should still exist (not consumed by friendly collision)
    EXPECT_TRUE(_ecs.entityIsActive(missile));
}

// ═══════════════════════════════════════════════════════════════════════════
// Enemy Missile Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(DamageSystemTest, PlayerTakesDamageFromEnemyMissile) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem collisionSystem(bridge);
    DamageSystem damageSystem(bridge, collisionSystem);

    auto player = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto enemyMissile = _ecs.entityCreate(ECS::EntityGroup::ENEMY_MISSILES);

    makeOverlapping(player, enemyMissile);

    auto& playerHealth = _ecs.entityAddComponent<HealthComp>(player);
    playerHealth.current = 100;
    playerHealth.max = 100;
    playerHealth.invulnerable = false;

    auto& missileTag = _ecs.entityAddComponent<MissileTag>(enemyMissile);
    missileTag.baseDamage = 15;

    collisionSystem.Update(_ecs, 0, 0);
    damageSystem.Update(_ecs, 0, 0);

    // Player should have taken damage
    EXPECT_EQ(playerHealth.current, 85);

    // Enemy missile should be deleted
    EXPECT_FALSE(_ecs.entityIsActive(enemyMissile));
}

TEST_F(DamageSystemTest, GodModeBlocksEnemyMissile) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem collisionSystem(bridge);
    DamageSystem damageSystem(bridge, collisionSystem);

    auto player = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);
    auto enemyMissile = _ecs.entityCreate(ECS::EntityGroup::ENEMY_MISSILES);

    makeOverlapping(player, enemyMissile);

    auto& playerHealth = _ecs.entityAddComponent<HealthComp>(player);
    playerHealth.current = 100;
    playerHealth.max = 100;
    playerHealth.invulnerable = true;  // GodMode

    _ecs.entityAddComponent<MissileTag>(enemyMissile);

    collisionSystem.Update(_ecs, 0, 0);
    damageSystem.Update(_ecs, 0, 0);

    // Player should NOT have taken damage
    EXPECT_EQ(playerHealth.current, 100);

    // Enemy missile should still be deleted (consumed)
    EXPECT_FALSE(_ecs.entityIsActive(enemyMissile));
}

// ═══════════════════════════════════════════════════════════════════════════
// Kill Events Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(DamageSystemTest, KillEventsClearedBetweenUpdates) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem collisionSystem(bridge);
    DamageSystem damageSystem(bridge, collisionSystem);

    // First kill
    auto missile1 = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto enemy1 = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
    makeOverlapping(missile1, enemy1);

    auto& missileTag1 = _ecs.entityAddComponent<MissileTag>(missile1);
    missileTag1.baseDamage = 50;
    auto& enemyHealth1 = _ecs.entityAddComponent<HealthComp>(enemy1);
    enemyHealth1.current = 40;
    _ecs.entityAddComponent<EnemyTag>(enemy1);

    collisionSystem.Update(_ecs, 0, 0);
    damageSystem.Update(_ecs, 0, 0);

    EXPECT_EQ(damageSystem.getKillEvents().size(), 1);

    // Second frame - no new kills
    collisionSystem.Update(_ecs, 0, 0);
    damageSystem.Update(_ecs, 0, 0);

    EXPECT_EQ(damageSystem.getKillEvents().size(), 0);
}

TEST_F(DamageSystemTest, ClearKillEventsManually) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);
    CollisionSystem collisionSystem(bridge);
    DamageSystem damageSystem(bridge, collisionSystem);

    auto missile = _ecs.entityCreate(ECS::EntityGroup::MISSILES);
    auto enemy = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);
    makeOverlapping(missile, enemy);

    auto& missileTag = _ecs.entityAddComponent<MissileTag>(missile);
    missileTag.baseDamage = 50;
    auto& enemyHealth = _ecs.entityAddComponent<HealthComp>(enemy);
    enemyHealth.current = 40;
    _ecs.entityAddComponent<EnemyTag>(enemy);

    collisionSystem.Update(_ecs, 0, 0);
    damageSystem.Update(_ecs, 0, 0);

    EXPECT_EQ(damageSystem.getKillEvents().size(), 1);

    damageSystem.clearKillEvents();

    EXPECT_EQ(damageSystem.getKillEvents().size(), 0);
}
