/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ComponentPool Tests - Verifies ECS component default values and operations
*/

#include <gtest/gtest.h>
#include "infrastructure/ecs/components/PositionComp.hpp"
#include "infrastructure/ecs/components/VelocityComp.hpp"
#include "infrastructure/ecs/components/HealthComp.hpp"
#include "infrastructure/ecs/components/HitboxComp.hpp"
#include "infrastructure/ecs/components/LifetimeComp.hpp"
#include "infrastructure/ecs/components/OwnerComp.hpp"

using namespace infrastructure::ecs::components;

// ═══════════════════════════════════════════════════════════════════════════
// PositionComp Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST(PositionCompTest, DefaultValues) {
    PositionComp pos;
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
}

TEST(PositionCompTest, CustomValues) {
    PositionComp pos{100.5f, 200.75f};
    EXPECT_FLOAT_EQ(pos.x, 100.5f);
    EXPECT_FLOAT_EQ(pos.y, 200.75f);
}

TEST(PositionCompTest, ModifyValues) {
    PositionComp pos;
    pos.x = 50.0f;
    pos.y = -30.5f;
    EXPECT_FLOAT_EQ(pos.x, 50.0f);
    EXPECT_FLOAT_EQ(pos.y, -30.5f);
}

// ═══════════════════════════════════════════════════════════════════════════
// VelocityComp Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST(VelocityCompTest, DefaultValues) {
    VelocityComp vel;
    EXPECT_FLOAT_EQ(vel.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
}

TEST(VelocityCompTest, NegativeVelocity) {
    VelocityComp vel{-120.0f, 0.0f};
    EXPECT_FLOAT_EQ(vel.x, -120.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// HealthComp Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST(HealthCompTest, DefaultValues) {
    HealthComp health;
    EXPECT_EQ(health.current, 100);
    EXPECT_EQ(health.max, 100);
    EXPECT_FALSE(health.invulnerable);
}

TEST(HealthCompTest, CustomValues) {
    HealthComp health{40, 40, false};
    EXPECT_EQ(health.current, 40);
    EXPECT_EQ(health.max, 40);
    EXPECT_FALSE(health.invulnerable);
}

TEST(HealthCompTest, Invulnerable) {
    HealthComp health{100, 100, true};
    EXPECT_TRUE(health.invulnerable);
}

TEST(HealthCompTest, DamagedState) {
    HealthComp health{30, 100, false};
    EXPECT_EQ(health.current, 30);
    EXPECT_EQ(health.max, 100);
}

// ═══════════════════════════════════════════════════════════════════════════
// HitboxComp Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST(HitboxCompTest, DefaultValues) {
    HitboxComp hitbox;
    EXPECT_FLOAT_EQ(hitbox.width, 0.0f);
    EXPECT_FLOAT_EQ(hitbox.height, 0.0f);
    EXPECT_FLOAT_EQ(hitbox.offsetX, 0.0f);
    EXPECT_FLOAT_EQ(hitbox.offsetY, 0.0f);
}

TEST(HitboxCompTest, ShipHitbox) {
    HitboxComp hitbox{50.0f, 30.0f, 0.0f, 0.0f};
    EXPECT_FLOAT_EQ(hitbox.width, 50.0f);
    EXPECT_FLOAT_EQ(hitbox.height, 30.0f);
}

TEST(HitboxCompTest, MissileHitbox) {
    HitboxComp hitbox{16.0f, 8.0f, 0.0f, 0.0f};
    EXPECT_FLOAT_EQ(hitbox.width, 16.0f);
    EXPECT_FLOAT_EQ(hitbox.height, 8.0f);
}

TEST(HitboxCompTest, WithOffset) {
    HitboxComp hitbox{32.0f, 32.0f, 5.0f, -5.0f};
    EXPECT_FLOAT_EQ(hitbox.offsetX, 5.0f);
    EXPECT_FLOAT_EQ(hitbox.offsetY, -5.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// LifetimeComp Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST(LifetimeCompTest, DefaultValues) {
    LifetimeComp lifetime;
    EXPECT_FLOAT_EQ(lifetime.remaining, 0.0f);
    EXPECT_FLOAT_EQ(lifetime.total, 0.0f);
}

TEST(LifetimeCompTest, PowerUpLifetime) {
    LifetimeComp lifetime{10.0f, 10.0f};
    EXPECT_FLOAT_EQ(lifetime.remaining, 10.0f);
    EXPECT_FLOAT_EQ(lifetime.total, 10.0f);
}

TEST(LifetimeCompTest, DecayingLifetime) {
    LifetimeComp lifetime{10.0f, 10.0f};
    lifetime.remaining -= 3.5f;
    EXPECT_FLOAT_EQ(lifetime.remaining, 6.5f);
    EXPECT_FLOAT_EQ(lifetime.total, 10.0f);  // Total unchanged
}

TEST(LifetimeCompTest, ExpiredLifetime) {
    LifetimeComp lifetime{0.5f, 10.0f};
    lifetime.remaining -= 1.0f;
    EXPECT_LT(lifetime.remaining, 0.0f);  // Expired
}

// ═══════════════════════════════════════════════════════════════════════════
// OwnerComp Tests
// ═══════════════════════════════════════════════════════════════════════════

TEST(OwnerCompTest, DefaultValues) {
    OwnerComp owner;
    EXPECT_EQ(owner.ownerId, 0);
    EXPECT_TRUE(owner.isPlayerOwned);
}

TEST(OwnerCompTest, PlayerMissile) {
    OwnerComp owner{1, true};
    EXPECT_EQ(owner.ownerId, 1);
    EXPECT_TRUE(owner.isPlayerOwned);
}

TEST(OwnerCompTest, EnemyMissile) {
    OwnerComp owner{100, false};
    EXPECT_EQ(owner.ownerId, 100);
    EXPECT_FALSE(owner.isPlayerOwned);
}

TEST(OwnerCompTest, NoOwner) {
    OwnerComp owner{0, true};
    EXPECT_EQ(owner.ownerId, 0);  // 0 = no owner
}
