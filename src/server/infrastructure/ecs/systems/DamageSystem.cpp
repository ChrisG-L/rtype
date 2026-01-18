/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** DamageSystem - Implementation
*/

#include "systems/DamageSystem.hpp"
#include "core/ECS.hpp"
#include "components/HealthComp.hpp"
#include "components/MissileTag.hpp"
#include "components/EnemyTag.hpp"
#include "components/OwnerComp.hpp"
#include "components/WaveCannonTag.hpp"
#include <vector>
#include <algorithm>

namespace infrastructure::ecs::systems {

    DamageSystem::DamageSystem(bridge::DomainBridge& bridge, CollisionSystem& collisionSystem)
        : _bridge(bridge), _collisionSystem(collisionSystem)
    {}

    void DamageSystem::Update(ECS::ECS& ecs, [[maybe_unused]] ECS::SystemID thisID, [[maybe_unused]] uint32_t msecs) {
        _killEvents.clear();

        const auto& collisions = _collisionSystem.getCollisions();
        std::vector<ECS::EntityID> toDelete;

        for (const auto& collision : collisions) {
            // Skip if either entity was already deleted this frame
            if (!ecs.entityIsActive(collision.entityA) || !ecs.entityIsActive(collision.entityB)) {
                continue;
            }

            processCollision(ecs, collision);
        }

        // Deferred deletion of dead entities
        // This is handled by checking if entities are still active
        // The actual deletion happens in the collision processing
    }

    void DamageSystem::processCollision(ECS::ECS& ecs, const CollisionEvent& collision) {
        ECS::EntityID entityA = collision.entityA;
        ECS::EntityID entityB = collision.entityB;
        ECS::EntityGroup groupA = collision.groupA;
        ECS::EntityGroup groupB = collision.groupB;

        // Helper to determine collision type
        auto hasGroup = [](ECS::EntityGroup g1, ECS::EntityGroup g2, ECS::EntityGroup target) {
            return g1 == target || g2 == target;
        };

        auto getEntityWithGroup = [&](ECS::EntityGroup target) -> ECS::EntityID {
            if (groupA == target) return entityA;
            if (groupB == target) return entityB;
            return 0;
        };

        auto getOtherEntity = [&](ECS::EntityID entity) -> ECS::EntityID {
            return (entity == entityA) ? entityB : entityA;
        };

        // ═══════════════════════════════════════════════════════════════════════
        // MISSILES (player) + ENEMIES → Enemy takes damage
        // ═══════════════════════════════════════════════════════════════════════
        if (hasGroup(groupA, groupB, ECS::EntityGroup::MISSILES) &&
            hasGroup(groupA, groupB, ECS::EntityGroup::ENEMIES)) {

            ECS::EntityID missile = getEntityWithGroup(ECS::EntityGroup::MISSILES);
            ECS::EntityID enemy = getEntityWithGroup(ECS::EntityGroup::ENEMIES);

            if (!ecs.entityHasComponent<components::HealthComp>(enemy)) {
                return;
            }

            uint8_t damage = getMissileDamage(ecs, missile);
            auto& health = ecs.entityGetComponent<components::HealthComp>(enemy);
            auto result = _bridge.applyDamage(health, damage);

            if (result.died) {
                // Record kill event
                KillEvent kill;
                kill.killerEntity = missile;
                kill.killedEntity = enemy;
                kill.killerPlayerId = getOwnerPlayerId(ecs, missile);
                if (ecs.entityHasComponent<components::EnemyTag>(enemy)) {
                    auto& enemyTag = ecs.entityGetComponent<components::EnemyTag>(enemy);
                    kill.killedType = enemyTag.type;
                    kill.basePoints = enemyTag.points;
                } else {
                    kill.killedType = 0;
                    kill.basePoints = 100;
                }
                _killEvents.push_back(kill);

                // Delete enemy
                ecs.entityDelete(enemy);
            }

            // Delete missile (consumed on hit)
            ecs.entityDelete(missile);
            return;
        }

        // ═══════════════════════════════════════════════════════════════════════
        // WAVE_CANNONS + ENEMIES → Enemy takes damage (wave cannon persists)
        // ═══════════════════════════════════════════════════════════════════════
        if (hasGroup(groupA, groupB, ECS::EntityGroup::WAVE_CANNONS) &&
            hasGroup(groupA, groupB, ECS::EntityGroup::ENEMIES)) {

            ECS::EntityID waveCannon = getEntityWithGroup(ECS::EntityGroup::WAVE_CANNONS);
            ECS::EntityID enemy = getEntityWithGroup(ECS::EntityGroup::ENEMIES);

            if (!ecs.entityHasComponent<components::HealthComp>(enemy)) {
                return;
            }

            // Get wave cannon damage from tag
            uint16_t damage = 50;  // Default level 1
            if (ecs.entityHasComponent<components::WaveCannonTag>(waveCannon)) {
                auto& wcTag = ecs.entityGetComponent<components::WaveCannonTag>(waveCannon);
                switch (wcTag.chargeLevel) {
                    case 1: damage = 50; break;
                    case 2: damage = 100; break;
                    case 3: damage = 250; break;
                }
            }

            auto& health = ecs.entityGetComponent<components::HealthComp>(enemy);
            auto result = _bridge.applyDamage(health, damage);

            if (result.died) {
                KillEvent kill;
                kill.killerEntity = waveCannon;
                kill.killedEntity = enemy;
                kill.killerPlayerId = getOwnerPlayerId(ecs, waveCannon);
                if (ecs.entityHasComponent<components::EnemyTag>(enemy)) {
                    auto& enemyTag = ecs.entityGetComponent<components::EnemyTag>(enemy);
                    kill.killedType = enemyTag.type;
                    kill.basePoints = enemyTag.points;
                } else {
                    kill.killedType = 0;
                    kill.basePoints = 100;
                }
                _killEvents.push_back(kill);

                ecs.entityDelete(enemy);
            }
            // Wave cannon does NOT get deleted (it persists through enemies)
            return;
        }

        // ═══════════════════════════════════════════════════════════════════════
        // PLAYERS + ENEMIES → Player takes contact damage
        // DISABLED: Enemy movement is still legacy, and this causes sync issues
        // with GameOver logic. Will be enabled when enemies are fully ECS-driven.
        // ═══════════════════════════════════════════════════════════════════════
        // if (hasGroup(groupA, groupB, ECS::EntityGroup::PLAYERS) &&
        //     hasGroup(groupA, groupB, ECS::EntityGroup::ENEMIES)) {
        //     // Contact damage disabled - legacy handles player damage
        // }

        // ═══════════════════════════════════════════════════════════════════════
        // PLAYERS + ENEMY_MISSILES → Player takes missile damage
        // ═══════════════════════════════════════════════════════════════════════
        if (hasGroup(groupA, groupB, ECS::EntityGroup::PLAYERS) &&
            hasGroup(groupA, groupB, ECS::EntityGroup::ENEMY_MISSILES)) {

            ECS::EntityID player = getEntityWithGroup(ECS::EntityGroup::PLAYERS);
            ECS::EntityID enemyMissile = getEntityWithGroup(ECS::EntityGroup::ENEMY_MISSILES);

            if (!ecs.entityHasComponent<components::HealthComp>(player)) {
                return;
            }

            if (isInvulnerable(ecs, player)) {
                ecs.entityDelete(enemyMissile);  // Missile consumed but no damage
                return;
            }

            uint8_t damage = getMissileDamage(ecs, enemyMissile);
            if (damage == 0) damage = 15;  // Default enemy missile damage

            auto& health = ecs.entityGetComponent<components::HealthComp>(player);
            auto result = _bridge.applyDamage(health, damage);

            ecs.entityDelete(enemyMissile);
            return;
        }

        // ═══════════════════════════════════════════════════════════════════════
        // PLAYERS + POWERUPS → Power-up collected (no damage, handled elsewhere)
        // ═══════════════════════════════════════════════════════════════════════
        // This is not a damage interaction, handled by a different system

        // ═══════════════════════════════════════════════════════════════════════
        // FORCE_PODS + ENEMIES → Enemy takes contact damage
        // ═══════════════════════════════════════════════════════════════════════
        if (hasGroup(groupA, groupB, ECS::EntityGroup::FORCE_PODS) &&
            hasGroup(groupA, groupB, ECS::EntityGroup::ENEMIES)) {

            ECS::EntityID forcePod = getEntityWithGroup(ECS::EntityGroup::FORCE_PODS);
            ECS::EntityID enemy = getEntityWithGroup(ECS::EntityGroup::ENEMIES);

            if (!ecs.entityHasComponent<components::HealthComp>(enemy)) {
                return;
            }

            constexpr uint8_t FORCE_CONTACT_DAMAGE = 30;
            auto& health = ecs.entityGetComponent<components::HealthComp>(enemy);
            auto result = _bridge.applyDamage(health, FORCE_CONTACT_DAMAGE);

            if (result.died) {
                KillEvent kill;
                kill.killerEntity = forcePod;
                kill.killedEntity = enemy;
                kill.killerPlayerId = getOwnerPlayerId(ecs, forcePod);
                if (ecs.entityHasComponent<components::EnemyTag>(enemy)) {
                    auto& enemyTag = ecs.entityGetComponent<components::EnemyTag>(enemy);
                    kill.killedType = enemyTag.type;
                    kill.basePoints = enemyTag.points;
                } else {
                    kill.killedType = 0;
                    kill.basePoints = 100;
                }
                _killEvents.push_back(kill);

                ecs.entityDelete(enemy);
            }
            // Force pod does NOT get deleted
            return;
        }

        // ═══════════════════════════════════════════════════════════════════════
        // MISSILES + MISSILES → No friendly fire
        // MISSILES + PLAYERS → No friendly fire (same team)
        // ═══════════════════════════════════════════════════════════════════════
        // These collisions are ignored
    }

    uint8_t DamageSystem::getMissileDamage(ECS::ECS& ecs, ECS::EntityID missile) {
        if (ecs.entityHasComponent<components::MissileTag>(missile)) {
            return ecs.entityGetComponent<components::MissileTag>(missile).baseDamage;
        }
        return 20;  // Default damage
    }

    uint8_t DamageSystem::getOwnerPlayerId(ECS::ECS& ecs, ECS::EntityID entity) {
        if (ecs.entityHasComponent<components::OwnerComp>(entity)) {
            return static_cast<uint8_t>(ecs.entityGetComponent<components::OwnerComp>(entity).ownerId);
        }
        return 0;
    }

    bool DamageSystem::isInvulnerable(ECS::ECS& ecs, ECS::EntityID entity) {
        if (ecs.entityHasComponent<components::HealthComp>(entity)) {
            return ecs.entityGetComponent<components::HealthComp>(entity).invulnerable;
        }
        return false;
    }

    const std::vector<KillEvent>& DamageSystem::getKillEvents() const {
        return _killEvents;
    }

    void DamageSystem::clearKillEvents() {
        _killEvents.clear();
    }

}  // namespace infrastructure::ecs::systems
