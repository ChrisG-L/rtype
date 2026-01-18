/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ECS Phase 4.8 Integration Tests - Hybrid ECS/Legacy system validation
*/

#include <gtest/gtest.h>
#include "infrastructure/ecs/core/ECS.hpp"

// All Components (Phase 1 + 2 + 3)
#include "infrastructure/ecs/components/PositionComp.hpp"
#include "infrastructure/ecs/components/VelocityComp.hpp"
#include "infrastructure/ecs/components/HealthComp.hpp"
#include "infrastructure/ecs/components/HitboxComp.hpp"
#include "infrastructure/ecs/components/LifetimeComp.hpp"
#include "infrastructure/ecs/components/OwnerComp.hpp"
#include "infrastructure/ecs/components/MissileTag.hpp"
#include "infrastructure/ecs/components/EnemyTag.hpp"
#include "infrastructure/ecs/components/EnemyAIComp.hpp"
#include "infrastructure/ecs/components/PowerUpTag.hpp"
#include "infrastructure/ecs/components/WaveCannonTag.hpp"
#include "infrastructure/ecs/components/PlayerTag.hpp"
#include "infrastructure/ecs/components/ScoreComp.hpp"
#include "infrastructure/ecs/components/WeaponComp.hpp"
#include "infrastructure/ecs/components/SpeedLevelComp.hpp"

// Systems
#include "infrastructure/ecs/systems/MovementSystem.hpp"
#include "infrastructure/ecs/systems/LifetimeSystem.hpp"
#include "infrastructure/ecs/systems/CleanupSystem.hpp"
#include "infrastructure/ecs/systems/CollisionSystem.hpp"
#include "infrastructure/ecs/systems/DamageSystem.hpp"
#include "infrastructure/ecs/systems/PlayerInputSystem.hpp"
#include "infrastructure/ecs/systems/ScoreSystem.hpp"

// Domain Bridge
#include "infrastructure/ecs/bridge/DomainBridge.hpp"
#include "domain/services/GameRule.hpp"
#include "domain/services/CollisionRule.hpp"
#include "domain/services/EnemyBehavior.hpp"

using namespace infrastructure::ecs::components;
using namespace infrastructure::ecs::systems;
using namespace infrastructure::ecs::bridge;

// =============================================================================
// Test Fixture - Full ECS Setup (All 15 Components)
// =============================================================================

class ECSPhase48IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register all 15 components
        _ecs.registerComponent<PositionComp>();
        _ecs.registerComponent<VelocityComp>();
        _ecs.registerComponent<HealthComp>();
        _ecs.registerComponent<HitboxComp>();
        _ecs.registerComponent<LifetimeComp>();
        _ecs.registerComponent<OwnerComp>();
        _ecs.registerComponent<MissileTag>();
        _ecs.registerComponent<EnemyTag>();
        _ecs.registerComponent<EnemyAIComp>();
        _ecs.registerComponent<PowerUpTag>();
        _ecs.registerComponent<WaveCannonTag>();
        _ecs.registerComponent<PlayerTag>();
        _ecs.registerComponent<ScoreComp>();
        _ecs.registerComponent<WeaponComp>();
        _ecs.registerComponent<SpeedLevelComp>();
    }

    // Helper: Create a full player entity (like GameWorld::createPlayerEntity)
    ECS::EntityID createPlayer(uint8_t playerId, float x, float y, uint8_t health = 100) {
        auto entity = _ecs.entityCreate(ECS::EntityGroup::PLAYERS);

        auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
        pos.x = x;
        pos.y = y;

        auto& vel = _ecs.entityAddComponent<VelocityComp>(entity);
        vel.x = 0.0f;
        vel.y = 0.0f;

        auto& hitbox = _ecs.entityAddComponent<HitboxComp>(entity);
        hitbox.width = 50.0f;
        hitbox.height = 30.0f;

        auto& healthComp = _ecs.entityAddComponent<HealthComp>(entity);
        healthComp.current = health;
        healthComp.max = 100;
        healthComp.invulnerable = false;

        auto& playerTag = _ecs.entityAddComponent<PlayerTag>(entity);
        playerTag.playerId = playerId;
        playerTag.shipSkin = 1;
        playerTag.isAlive = true;

        auto& score = _ecs.entityAddComponent<ScoreComp>(entity);
        score.total = 0;
        score.kills = 0;
        score.comboMultiplier = 1.0f;
        score.comboTimer = 0.0f;

        auto& weapon = _ecs.entityAddComponent<WeaponComp>(entity);
        weapon.currentType = 0;
        weapon.shootCooldown = 0.0f;

        auto& speed = _ecs.entityAddComponent<SpeedLevelComp>(entity);
        speed.level = 0;

        return entity;
    }

    // Helper: Create a missile entity
    ECS::EntityID createMissile(float x, float y, float velX, uint8_t damage, uint8_t ownerId) {
        auto entity = _ecs.entityCreate(ECS::EntityGroup::MISSILES);

        auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
        pos.x = x;
        pos.y = y;

        auto& vel = _ecs.entityAddComponent<VelocityComp>(entity);
        vel.x = velX;
        vel.y = 0.0f;

        auto& hitbox = _ecs.entityAddComponent<HitboxComp>(entity);
        hitbox.width = 16.0f;
        hitbox.height = 8.0f;

        auto& tag = _ecs.entityAddComponent<MissileTag>(entity);
        tag.baseDamage = damage;
        tag.weaponType = 0;

        auto& owner = _ecs.entityAddComponent<OwnerComp>(entity);
        owner.ownerId = ownerId;
        owner.isPlayerOwned = true;

        auto& lifetime = _ecs.entityAddComponent<LifetimeComp>(entity);
        lifetime.remaining = 10.0f;
        lifetime.total = 10.0f;

        return entity;
    }

    // Helper: Create an enemy entity
    ECS::EntityID createEnemy(float x, float y, float velX, uint8_t health, uint8_t type, uint16_t points) {
        auto entity = _ecs.entityCreate(ECS::EntityGroup::ENEMIES);

        auto& pos = _ecs.entityAddComponent<PositionComp>(entity);
        pos.x = x;
        pos.y = y;

        auto& vel = _ecs.entityAddComponent<VelocityComp>(entity);
        vel.x = velX;
        vel.y = 0.0f;

        auto& hitbox = _ecs.entityAddComponent<HitboxComp>(entity);
        hitbox.width = 40.0f;
        hitbox.height = 40.0f;

        auto& healthComp = _ecs.entityAddComponent<HealthComp>(entity);
        healthComp.current = health;
        healthComp.max = health;

        auto& tag = _ecs.entityAddComponent<EnemyTag>(entity);
        tag.type = type;
        tag.points = points;

        auto& ai = _ecs.entityAddComponent<EnemyAIComp>(entity);
        ai.shootCooldown = 2.0f;
        ai.shootInterval = 2.5f;
        ai.movementPattern = type;

        return entity;
    }

    ECS::ECS _ecs;

    // Domain services for bridge
    domain::services::GameRule _gameRule;
    domain::services::CollisionRule _collisionRule;
    domain::services::EnemyBehavior _enemyBehavior;
};

// =============================================================================
// Test: All 15 Components Registered
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, All15ComponentsRegistered) {
    // Phase 1 components
    EXPECT_TRUE(_ecs.componentExists<PositionComp>());
    EXPECT_TRUE(_ecs.componentExists<VelocityComp>());
    EXPECT_TRUE(_ecs.componentExists<HealthComp>());
    EXPECT_TRUE(_ecs.componentExists<HitboxComp>());
    EXPECT_TRUE(_ecs.componentExists<LifetimeComp>());
    EXPECT_TRUE(_ecs.componentExists<OwnerComp>());

    // Phase 2 components
    EXPECT_TRUE(_ecs.componentExists<MissileTag>());
    EXPECT_TRUE(_ecs.componentExists<EnemyTag>());
    EXPECT_TRUE(_ecs.componentExists<EnemyAIComp>());
    EXPECT_TRUE(_ecs.componentExists<PowerUpTag>());
    EXPECT_TRUE(_ecs.componentExists<WaveCannonTag>());

    // Phase 3 components
    EXPECT_TRUE(_ecs.componentExists<PlayerTag>());
    EXPECT_TRUE(_ecs.componentExists<ScoreComp>());
    EXPECT_TRUE(_ecs.componentExists<WeaponComp>());
    EXPECT_TRUE(_ecs.componentExists<SpeedLevelComp>());
}

// =============================================================================
// Test: Player Entity Lifecycle
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, PlayerEntityLifecycle) {
    // Create player
    auto player = createPlayer(1, 100.0f, 300.0f);

    EXPECT_TRUE(_ecs.entityIsActive(player));

    // Verify all components present
    EXPECT_TRUE(_ecs.entityHasComponent<PositionComp>(player));
    EXPECT_TRUE(_ecs.entityHasComponent<VelocityComp>(player));
    EXPECT_TRUE(_ecs.entityHasComponent<HealthComp>(player));
    EXPECT_TRUE(_ecs.entityHasComponent<HitboxComp>(player));
    EXPECT_TRUE(_ecs.entityHasComponent<PlayerTag>(player));
    EXPECT_TRUE(_ecs.entityHasComponent<ScoreComp>(player));
    EXPECT_TRUE(_ecs.entityHasComponent<WeaponComp>(player));
    EXPECT_TRUE(_ecs.entityHasComponent<SpeedLevelComp>(player));

    // Verify initial values
    auto& playerTag = _ecs.entityGetComponent<PlayerTag>(player);
    EXPECT_EQ(playerTag.playerId, 1);
    EXPECT_TRUE(playerTag.isAlive);

    auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    EXPECT_EQ(score.total, 0);
    EXPECT_EQ(score.kills, 0);
    EXPECT_NEAR(score.comboMultiplier, 1.0f, 0.01f);

    auto& speed = _ecs.entityGetComponent<SpeedLevelComp>(player);
    EXPECT_EQ(speed.level, 0);

    // Delete player
    _ecs.entityDelete(player);
    EXPECT_FALSE(_ecs.entityIsActive(player));
}

// =============================================================================
// Test: Player Movement Through ECS (Input -> Velocity -> Position)
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, PlayerMovementThroughECS) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);

    PlayerInputSystem inputSystem(bridge);
    MovementSystem movementSystem;

    auto player = createPlayer(1, 100.0f, 300.0f);

    // Queue input: move right
    inputSystem.queueInput({.playerId = 1, .keys = InputKeys::RIGHT, .sequenceNum = 1});

    // Process input (sets velocity)
    inputSystem.Update(_ecs, 0, 16);

    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);
    EXPECT_GT(vel.x, 0.0f);  // Should have positive X velocity

    // Movement system applies velocity
    movementSystem.Update(_ecs, 0, 100);  // 100ms

    auto& pos = _ecs.entityGetComponent<PositionComp>(player);
    EXPECT_GT(pos.x, 100.0f);  // Should have moved right
}

// =============================================================================
// Test: Player Input All Directions
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, PlayerInputAllDirections) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);

    PlayerInputSystem inputSystem(bridge);
    MovementSystem movementSystem;

    auto player = createPlayer(1, 500.0f, 400.0f);
    inputSystem.setClampEnabled(false);  // Disable clamping for test

    // Test UP
    inputSystem.queueInput({.playerId = 1, .keys = InputKeys::UP, .sequenceNum = 1});
    inputSystem.Update(_ecs, 0, 16);
    auto& vel = _ecs.entityGetComponent<VelocityComp>(player);
    EXPECT_LT(vel.y, 0.0f);  // UP = negative Y

    // Reset and test DOWN
    vel.x = 0.0f; vel.y = 0.0f;
    inputSystem.queueInput({.playerId = 1, .keys = InputKeys::DOWN, .sequenceNum = 2});
    inputSystem.Update(_ecs, 0, 16);
    EXPECT_GT(vel.y, 0.0f);  // DOWN = positive Y

    // Reset and test LEFT
    vel.x = 0.0f; vel.y = 0.0f;
    inputSystem.queueInput({.playerId = 1, .keys = InputKeys::LEFT, .sequenceNum = 3});
    inputSystem.Update(_ecs, 0, 16);
    EXPECT_LT(vel.x, 0.0f);  // LEFT = negative X

    // Test diagonal (UP + RIGHT)
    vel.x = 0.0f; vel.y = 0.0f;
    inputSystem.queueInput({.playerId = 1, .keys = InputKeys::UP | InputKeys::RIGHT, .sequenceNum = 4});
    inputSystem.Update(_ecs, 0, 16);
    EXPECT_GT(vel.x, 0.0f);
    EXPECT_LT(vel.y, 0.0f);
}

// =============================================================================
// Test: Speed Level Affects Movement Speed
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, SpeedLevelAffectsMovement) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);

    PlayerInputSystem inputSystem(bridge);
    MovementSystem movementSystem;
    inputSystem.setClampEnabled(false);

    // Create two players with different speed levels
    auto player0 = createPlayer(1, 100.0f, 300.0f);
    auto player1 = createPlayer(2, 100.0f, 500.0f);

    auto& speed0 = _ecs.entityGetComponent<SpeedLevelComp>(player0);
    speed0.level = 0;  // Base speed

    auto& speed1 = _ecs.entityGetComponent<SpeedLevelComp>(player1);
    speed1.level = 3;  // Max speed

    // Both move right
    inputSystem.queueInput({.playerId = 1, .keys = InputKeys::RIGHT, .sequenceNum = 1});
    inputSystem.queueInput({.playerId = 2, .keys = InputKeys::RIGHT, .sequenceNum = 1});
    inputSystem.Update(_ecs, 0, 16);

    auto& vel0 = _ecs.entityGetComponent<VelocityComp>(player0);
    auto& vel1 = _ecs.entityGetComponent<VelocityComp>(player1);

    // Player with speed level 3 should have higher velocity
    EXPECT_GT(vel1.x, vel0.x);
    EXPECT_NEAR(vel1.x / vel0.x, 1.9f, 0.1f);  // 1.9x speed at level 3
}

// =============================================================================
// Test: Missile Entity with Lifetime
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, MissileEntityWithLifetime) {
    LifetimeSystem lifetimeSystem;
    MovementSystem movementSystem;

    auto missile = createMissile(100.0f, 300.0f, 600.0f, 20, 1);

    EXPECT_TRUE(_ecs.entityIsActive(missile));
    EXPECT_TRUE(_ecs.entityHasComponent<LifetimeComp>(missile));

    auto& lifetime = _ecs.entityGetComponent<LifetimeComp>(missile);
    EXPECT_NEAR(lifetime.remaining, 10.0f, 0.01f);

    // Simulate 5 seconds
    lifetimeSystem.Update(_ecs, 0, 5000);

    auto& lt = _ecs.entityGetComponent<LifetimeComp>(missile);
    EXPECT_NEAR(lt.remaining, 5.0f, 0.01f);
    EXPECT_TRUE(_ecs.entityIsActive(missile));

    // Simulate 6 more seconds (total > 10)
    lifetimeSystem.Update(_ecs, 0, 6000);

    EXPECT_FALSE(_ecs.entityIsActive(missile));
}

// =============================================================================
// Test: Enemy Entity with AI Component
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, EnemyEntityWithAIComponent) {
    auto enemy = createEnemy(800.0f, 300.0f, -120.0f, 50, 0, 100);

    EXPECT_TRUE(_ecs.entityIsActive(enemy));
    EXPECT_TRUE(_ecs.entityHasComponent<EnemyAIComp>(enemy));
    EXPECT_TRUE(_ecs.entityHasComponent<EnemyTag>(enemy));

    auto& ai = _ecs.entityGetComponent<EnemyAIComp>(enemy);
    EXPECT_NEAR(ai.shootCooldown, 2.0f, 0.01f);
    EXPECT_NEAR(ai.shootInterval, 2.5f, 0.01f);
    EXPECT_EQ(ai.movementPattern, 0);

    auto& tag = _ecs.entityGetComponent<EnemyTag>(enemy);
    EXPECT_EQ(tag.type, 0);
    EXPECT_EQ(tag.points, 100);
}

// NOTE: PlayerEnemyCollisionIntegration test removed
// R-Type design: NO player-enemy contact damage (confirmed in legacy code)
// Damage only comes from: enemy missiles, boss attacks

// =============================================================================
// Test: Multiple Players Independent State
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, MultiplePlayersIndependentState) {
    auto player1 = createPlayer(1, 100.0f, 200.0f);
    auto player2 = createPlayer(2, 100.0f, 400.0f);
    auto player3 = createPlayer(3, 100.0f, 600.0f);

    // Modify player 1 state
    auto& score1 = _ecs.entityGetComponent<ScoreComp>(player1);
    score1.total = 5000;
    score1.kills = 10;

    auto& speed2 = _ecs.entityGetComponent<SpeedLevelComp>(player2);
    speed2.level = 2;

    auto& weapon3 = _ecs.entityGetComponent<WeaponComp>(player3);
    weapon3.currentType = 2;  // Laser

    // Verify independence
    auto& score2 = _ecs.entityGetComponent<ScoreComp>(player2);
    EXPECT_EQ(score2.total, 0);  // Unchanged

    auto& speed1 = _ecs.entityGetComponent<SpeedLevelComp>(player1);
    EXPECT_EQ(speed1.level, 0);  // Unchanged

    auto& weapon1 = _ecs.entityGetComponent<WeaponComp>(player1);
    EXPECT_EQ(weapon1.currentType, 0);  // Unchanged
}

// =============================================================================
// Test: Score System Kill Event Processing
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, ScoreSystemKillEventProcessing) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);

    ScoreSystem scoreSystem(bridge);

    auto player = createPlayer(1, 100.0f, 300.0f);

    // Queue kill events
    scoreSystem.queueKill({.playerId = 1, .basePoints = 100, .enemyType = 0});
    scoreSystem.queueKill({.playerId = 1, .basePoints = 150, .enemyType = 1});

    // Process scores
    scoreSystem.Update(_ecs, 0, 16);

    auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    EXPECT_GT(score.total, 0);
    EXPECT_EQ(score.kills, 2);
    EXPECT_GT(score.comboMultiplier, 1.0f);  // Combo should have increased
}

// =============================================================================
// Test: Combo Decay Over Time
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, ComboDecayOverTime) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);

    ScoreSystem scoreSystem(bridge);

    auto player = createPlayer(1, 100.0f, 300.0f);

    // Get a kill to start combo
    scoreSystem.queueKill({.playerId = 1, .basePoints = 100, .enemyType = 0});
    scoreSystem.Update(_ecs, 0, 16);

    auto& score = _ecs.entityGetComponent<ScoreComp>(player);
    float initialCombo = score.comboMultiplier;
    EXPECT_GT(initialCombo, 1.0f);

    // Wait for combo grace period to expire (> 3 seconds)
    for (int i = 0; i < 50; ++i) {  // 50 * 100ms = 5 seconds
        scoreSystem.Update(_ecs, 0, 100);
    }

    // Combo should have decayed back to 1.0
    EXPECT_NEAR(score.comboMultiplier, 1.0f, 0.1f);
}

// =============================================================================
// Test: Query Entities By Components
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, QueryEntitiesByComponents) {
    createPlayer(1, 100.0f, 200.0f);
    createPlayer(2, 100.0f, 400.0f);
    createMissile(500.0f, 200.0f, 600.0f, 20, 1);
    createMissile(500.0f, 400.0f, 600.0f, 20, 2);
    createEnemy(800.0f, 300.0f, -120.0f, 50, 0, 100);

    // Query players
    auto players = _ecs.getEntitiesByComponentsAllOf<PlayerTag, PositionComp, HealthComp>();
    EXPECT_EQ(players.size(), 2);

    // Query missiles
    auto missiles = _ecs.getEntitiesByComponentsAllOf<MissileTag, PositionComp, VelocityComp>();
    EXPECT_EQ(missiles.size(), 2);

    // Query enemies
    auto enemies = _ecs.getEntitiesByComponentsAllOf<EnemyTag, EnemyAIComp>();
    EXPECT_EQ(enemies.size(), 1);

    // Query any with health (players + enemy = 3)
    auto withHealth = _ecs.getEntitiesByComponentsAllOf<HealthComp>();
    EXPECT_EQ(withHealth.size(), 3);
}

// =============================================================================
// Test: Full Game Loop Simulation (100 frames)
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, FullGameLoopSimulation100Frames) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);

    PlayerInputSystem inputSystem(bridge);
    MovementSystem movementSystem;
    LifetimeSystem lifetimeSystem;
    CleanupSystem cleanupSystem(bridge);
    CollisionSystem collisionSystem(bridge);
    DamageSystem damageSystem(bridge, collisionSystem);
    ScoreSystem scoreSystem(bridge);

    // Create game entities
    auto player = createPlayer(1, 100.0f, 400.0f);
    createMissile(150.0f, 400.0f, 600.0f, 30, 1);
    createEnemy(800.0f, 400.0f, -150.0f, 30, 0, 100);

    // Simulate 100 frames at 60 FPS (16ms each)
    uint32_t totalKills = 0;
    for (int frame = 0; frame < 100; ++frame) {
        // Player moves right occasionally
        if (frame % 10 == 0) {
            inputSystem.queueInput({.playerId = 1, .keys = InputKeys::RIGHT, .sequenceNum = static_cast<uint16_t>(frame)});
        }

        // Run all systems in priority order
        inputSystem.Update(_ecs, 0, 16);
        movementSystem.Update(_ecs, 0, 16);
        collisionSystem.Update(_ecs, 0, 16);
        damageSystem.Update(_ecs, 0, 16);
        lifetimeSystem.Update(_ecs, 0, 16);
        cleanupSystem.Update(_ecs, 0, 16);
        scoreSystem.Update(_ecs, 0, 16);

        totalKills += damageSystem.getKillEvents().size();
        damageSystem.clearKillEvents();
    }

    // Player should still be alive
    EXPECT_TRUE(_ecs.entityIsActive(player));

    // Simulation completes without crashes
    SUCCEED();
}

// =============================================================================
// Test: Entity Deletion Cascades Correctly
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, EntityDeletionCascadesCorrectly) {
    auto player = createPlayer(1, 100.0f, 300.0f);
    auto missile1 = createMissile(200.0f, 300.0f, 600.0f, 20, 1);
    auto missile2 = createMissile(300.0f, 300.0f, 600.0f, 20, 1);

    EXPECT_EQ(_ecs.currentEntityCount(), 3);

    // Delete player
    _ecs.entityDelete(player);
    EXPECT_EQ(_ecs.currentEntityCount(), 2);

    // Delete missiles
    _ecs.entityDelete(missile1);
    _ecs.entityDelete(missile2);
    EXPECT_EQ(_ecs.currentEntityCount(), 0);
}

// =============================================================================
// Test: Weapon Component State Management
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, WeaponComponentStateManagement) {
    auto player = createPlayer(1, 100.0f, 300.0f);

    auto& weapon = _ecs.entityGetComponent<WeaponComp>(player);

    // Initial state
    EXPECT_EQ(weapon.currentType, 0);
    EXPECT_EQ(weapon.getCurrentLevel(), 0);
    EXPECT_FALSE(weapon.isCharging);

    // Upgrade weapon
    EXPECT_TRUE(weapon.upgradeCurrentWeapon());
    EXPECT_EQ(weapon.getCurrentLevel(), 1);

    EXPECT_TRUE(weapon.upgradeCurrentWeapon());
    EXPECT_EQ(weapon.getCurrentLevel(), 2);

    EXPECT_TRUE(weapon.upgradeCurrentWeapon());
    EXPECT_EQ(weapon.getCurrentLevel(), 3);

    // Max level reached
    EXPECT_FALSE(weapon.upgradeCurrentWeapon());
    EXPECT_EQ(weapon.getCurrentLevel(), 3);

    // Switch weapon
    weapon.currentType = 1;  // Spread
    EXPECT_EQ(weapon.getCurrentLevel(), 0);  // New weapon starts at level 0
}

// =============================================================================
// Test: Speed Level Component Upgrade
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, SpeedLevelComponentUpgrade) {
    auto player = createPlayer(1, 100.0f, 300.0f);

    auto& speed = _ecs.entityGetComponent<SpeedLevelComp>(player);

    EXPECT_EQ(speed.level, 0);
    EXPECT_FALSE(speed.isMaxLevel());

    EXPECT_TRUE(speed.upgrade());
    EXPECT_EQ(speed.level, 1);

    EXPECT_TRUE(speed.upgrade());
    EXPECT_EQ(speed.level, 2);

    EXPECT_TRUE(speed.upgrade());
    EXPECT_EQ(speed.level, 3);
    EXPECT_TRUE(speed.isMaxLevel());

    // Can't upgrade past max
    EXPECT_FALSE(speed.upgrade());
    EXPECT_EQ(speed.level, 3);
}

// =============================================================================
// Test: Systems Execute Without Errors (Stress Test)
// =============================================================================

TEST_F(ECSPhase48IntegrationTest, SystemsStressTest200Frames) {
    DomainBridge bridge(_gameRule, _collisionRule, _enemyBehavior);

    PlayerInputSystem inputSystem(bridge);
    MovementSystem movementSystem;
    LifetimeSystem lifetimeSystem;
    CleanupSystem cleanupSystem(bridge);
    CollisionSystem collisionSystem(bridge);
    DamageSystem damageSystem(bridge, collisionSystem);
    ScoreSystem scoreSystem(bridge);

    // Create many entities
    for (int i = 0; i < 4; ++i) {
        createPlayer(i + 1, 100.0f + i * 50, 200.0f + i * 100);
    }
    for (int i = 0; i < 16; ++i) {
        createEnemy(800.0f + i * 30, 100.0f + i * 50, -120.0f, 40, i % 5, 100 + i * 10);
    }
    for (int i = 0; i < 32; ++i) {
        createMissile(200.0f + i * 20, 100.0f + i * 25, 600.0f, 20, (i % 4) + 1);
    }

    // Run 200 frames
    for (int frame = 0; frame < 200; ++frame) {
        inputSystem.Update(_ecs, 0, 16);
        movementSystem.Update(_ecs, 0, 16);
        collisionSystem.Update(_ecs, 0, 16);
        damageSystem.Update(_ecs, 0, 16);
        lifetimeSystem.Update(_ecs, 0, 16);
        cleanupSystem.Update(_ecs, 0, 16);
        scoreSystem.Update(_ecs, 0, 16);
    }

    // Test passes if no crash
    SUCCEED();
}
