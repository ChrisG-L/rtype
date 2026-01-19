/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** CollisionSystem - Implementation (Optimized)
*/

#include "systems/CollisionSystem.hpp"
#include "core/ECS.hpp"
#include "components/PositionComp.hpp"
#include "components/HitboxComp.hpp"

namespace infrastructure::ecs::systems {

    CollisionSystem::CollisionSystem(bridge::DomainBridge& bridge)
        : _bridge(bridge)
    {}

    void CollisionSystem::Update(ECS::ECS& ecs, [[maybe_unused]] ECS::SystemID thisID, [[maybe_unused]] uint32_t msecs) {
        _collisions.clear();

        // Fetch entity groups ONCE per frame (avoiding repeated lookups)
        auto missiles = ecs.getEntityGroup(ECS::EntityGroup::MISSILES);
        auto enemies = ecs.getEntityGroup(ECS::EntityGroup::ENEMIES);
        auto players = ecs.getEntityGroup(ECS::EntityGroup::PLAYERS);
        auto enemyMissiles = ecs.getEntityGroup(ECS::EntityGroup::ENEMY_MISSILES);
        auto waveCannons = ecs.getEntityGroup(ECS::EntityGroup::WAVE_CANNONS);
        auto forcePods = ecs.getEntityGroup(ECS::EntityGroup::FORCE_PODS);

        // Only check RELEVANT collision pairs (like legacy GameWorld)
        // This avoids checking missiles vs missiles, enemies vs enemies, etc.

        // 1. MISSILES vs ENEMIES → Enemy takes damage
        checkPairs(ecs, missiles, enemies,
                   ECS::EntityGroup::MISSILES, ECS::EntityGroup::ENEMIES);

        // 2. WAVE_CANNONS vs ENEMIES → Enemy takes damage (wave cannon persists)
        checkPairs(ecs, waveCannons, enemies,
                   ECS::EntityGroup::WAVE_CANNONS, ECS::EntityGroup::ENEMIES);

        // 3. PLAYERS vs ENEMY_MISSILES → Player takes damage
        checkPairs(ecs, players, enemyMissiles,
                   ECS::EntityGroup::PLAYERS, ECS::EntityGroup::ENEMY_MISSILES);

        // 4. FORCE_PODS vs ENEMIES → Enemy takes contact damage
        checkPairs(ecs, forcePods, enemies,
                   ECS::EntityGroup::FORCE_PODS, ECS::EntityGroup::ENEMIES);

        // 5. PLAYERS vs POWERUPS → Player collects power-up
        auto powerups = ecs.getEntityGroup(ECS::EntityGroup::POWERUPS);
        checkPairs(ecs, players, powerups,
                   ECS::EntityGroup::PLAYERS, ECS::EntityGroup::POWERUPS);
    }

    void CollisionSystem::checkPairs(ECS::ECS& ecs,
                                     const std::vector<ECS::EntityID>& groupA,
                                     const std::vector<ECS::EntityID>& groupB,
                                     ECS::EntityGroup typeA,
                                     ECS::EntityGroup typeB) {
        for (auto entityA : groupA) {
            // Skip if entity was deleted or doesn't have required components
            if (!ecs.entityIsActive(entityA) ||
                !ecs.entityHasComponent<components::PositionComp>(entityA) ||
                !ecs.entityHasComponent<components::HitboxComp>(entityA)) {
                continue;
            }

            const auto& posA = ecs.entityGetComponent<components::PositionComp>(entityA);
            const auto& hitboxA = ecs.entityGetComponent<components::HitboxComp>(entityA);

            float x1 = posA.x + hitboxA.offsetX;
            float y1 = posA.y + hitboxA.offsetY;

            for (auto entityB : groupB) {
                // Skip if entity was deleted or doesn't have required components
                if (!ecs.entityIsActive(entityB) ||
                    !ecs.entityHasComponent<components::PositionComp>(entityB) ||
                    !ecs.entityHasComponent<components::HitboxComp>(entityB)) {
                    continue;
                }

                const auto& posB = ecs.entityGetComponent<components::PositionComp>(entityB);
                const auto& hitboxB = ecs.entityGetComponent<components::HitboxComp>(entityB);

                float x2 = posB.x + hitboxB.offsetX;
                float y2 = posB.y + hitboxB.offsetY;

                if (_bridge.checkCollision(x1, y1, hitboxA.width, hitboxA.height,
                                          x2, y2, hitboxB.width, hitboxB.height)) {
                    _collisions.push_back({entityA, entityB, typeA, typeB});
                }
            }
        }
    }

    const std::vector<CollisionEvent>& CollisionSystem::getCollisions() const {
        return _collisions;
    }

    void CollisionSystem::clearCollisions() {
        _collisions.clear();
    }

}  // namespace infrastructure::ecs::systems
