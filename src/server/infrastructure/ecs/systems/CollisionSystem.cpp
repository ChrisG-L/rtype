/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** CollisionSystem - Implementation
*/

#include "systems/CollisionSystem.hpp"
#include "core/ECS.hpp"
#include "components/PositionComp.hpp"
#include "components/HitboxComp.hpp"
#include <algorithm>

namespace infrastructure::ecs::systems {

    CollisionSystem::CollisionSystem(bridge::DomainBridge& bridge)
        : _bridge(bridge)
    {}

    void CollisionSystem::Update(ECS::ECS& ecs, [[maybe_unused]] ECS::SystemID thisID, [[maybe_unused]] uint32_t msecs) {
        // Clear previous frame's collisions
        _collisions.clear();

        // Query all entities with Position and Hitbox components
        auto entities = ecs.getEntitiesByComponentsAllOf<
            components::PositionComp,
            components::HitboxComp
        >();

        // O(n²) collision detection
        // TODO: Optimize with spatial hashing if entity count exceeds ~500
        for (size_t i = 0; i < entities.size(); ++i) {
            for (size_t j = i + 1; j < entities.size(); ++j) {
                ECS::EntityID entityA = entities[i];
                ECS::EntityID entityB = entities[j];

                // Get components
                const auto& posA = ecs.entityGetComponent<components::PositionComp>(entityA);
                const auto& hitboxA = ecs.entityGetComponent<components::HitboxComp>(entityA);
                const auto& posB = ecs.entityGetComponent<components::PositionComp>(entityB);
                const auto& hitboxB = ecs.entityGetComponent<components::HitboxComp>(entityB);

                // Calculate actual bounds (position + hitbox offset)
                float x1 = posA.x + hitboxA.offsetX;
                float y1 = posA.y + hitboxA.offsetY;
                float x2 = posB.x + hitboxB.offsetX;
                float y2 = posB.y + hitboxB.offsetY;

                // Check collision via DomainBridge
                if (_bridge.checkCollision(x1, y1, hitboxA.width, hitboxA.height,
                                          x2, y2, hitboxB.width, hitboxB.height)) {
                    // Record collision event
                    CollisionEvent event;
                    event.entityA = entityA;
                    event.entityB = entityB;
                    event.groupA = getEntityGroup(ecs, entityA);
                    event.groupB = getEntityGroup(ecs, entityB);
                    _collisions.push_back(event);
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

    ECS::EntityGroup CollisionSystem::getEntityGroup(ECS::ECS& ecs, ECS::EntityID entity) {
        // Check each group
        static const ECS::EntityGroup groups[] = {
            ECS::EntityGroup::PLAYERS,
            ECS::EntityGroup::ENEMIES,
            ECS::EntityGroup::MISSILES,
            ECS::EntityGroup::ENEMY_MISSILES,
            ECS::EntityGroup::POWERUPS,
            ECS::EntityGroup::WAVE_CANNONS,
            ECS::EntityGroup::FORCE_PODS,
            ECS::EntityGroup::BIT_DEVICES,
            ECS::EntityGroup::BOSS
        };

        for (auto group : groups) {
            auto groupEntities = ecs.getEntityGroup(group);
            if (std::find(groupEntities.begin(), groupEntities.end(), entity) != groupEntities.end()) {
                return group;
            }
        }

        return ECS::EntityGroup::NONE;
    }

}  // namespace infrastructure::ecs::systems
