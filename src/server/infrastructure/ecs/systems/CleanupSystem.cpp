/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** CleanupSystem - Implementation
*/

#include "systems/CleanupSystem.hpp"
#include "core/ECS.hpp"
#include "components/PositionComp.hpp"
#include "components/HitboxComp.hpp"
#include <vector>

namespace infrastructure::ecs::systems {

    CleanupSystem::CleanupSystem(bridge::DomainBridge& bridge)
        : _bridge(bridge)
    {}

    void CleanupSystem::Update(ECS::ECS& ecs, [[maybe_unused]] ECS::SystemID thisID, [[maybe_unused]] uint32_t msecs) {
        // Query all entities with Position and Hitbox components
        auto entities = ecs.getEntitiesByComponentsAllOf<
            components::PositionComp,
            components::HitboxComp
        >();

        // Collect entities to delete (deferred deletion)
        std::vector<ECS::EntityID> toDelete;

        // Get groups to exclude from OOB cleanup
        // Players: handled by respawn logic
        // Enemies: spawn at x=SCREEN_WIDTH, have their own OOB logic in updateEnemies()
        auto players = ecs.getEntityGroup(ECS::EntityGroup::PLAYERS);
        auto enemies = ecs.getEntityGroup(ECS::EntityGroup::ENEMIES);

        for (auto entityId : entities) {
            // Skip players - they are handled by respawn logic
            bool isExcluded = false;
            for (auto playerId : players) {
                if (entityId == playerId) {
                    isExcluded = true;
                    break;
                }
            }
            // Skip enemies - they spawn at x=SCREEN_WIDTH and exit left
            // Their OOB check is in updateEnemies() (x < -Enemy::WIDTH)
            if (!isExcluded) {
                for (auto enemyId : enemies) {
                    if (entityId == enemyId) {
                        isExcluded = true;
                        break;
                    }
                }
            }
            if (isExcluded) {
                continue;
            }

            const auto& pos = ecs.entityGetComponent<components::PositionComp>(entityId);
            const auto& hitbox = ecs.entityGetComponent<components::HitboxComp>(entityId);

            // Calculate actual bounds (position + hitbox offset)
            float actualX = pos.x + hitbox.offsetX;
            float actualY = pos.y + hitbox.offsetY;

            // Check if entity is fully out of bounds
            if (_bridge.isOutOfBounds(actualX, actualY, hitbox.width, hitbox.height)) {
                toDelete.push_back(entityId);
            }
        }

        // Perform deferred deletion
        for (auto entityId : toDelete) {
            ecs.entityDelete(entityId);
        }
    }

}  // namespace infrastructure::ecs::systems
