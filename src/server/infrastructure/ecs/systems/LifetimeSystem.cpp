/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LifetimeSystem - Implementation
*/

#include "systems/LifetimeSystem.hpp"
#include "core/ECS.hpp"
#include "components/LifetimeComp.hpp"
#include <vector>

namespace infrastructure::ecs::systems {

    void LifetimeSystem::Update(ECS::ECS& ecs, [[maybe_unused]] ECS::SystemID thisID, uint32_t msecs) {
        // Convert milliseconds to seconds
        float deltaTime = static_cast<float>(msecs) / 1000.0f;

        // Query all entities with Lifetime component
        auto entities = ecs.getEntitiesByComponentsAllOf<components::LifetimeComp>();

        // Collect entities to delete (deferred deletion)
        std::vector<ECS::EntityID> toDelete;

        for (auto entityId : entities) {
            auto& lifetime = ecs.entityGetComponent<components::LifetimeComp>(entityId);

            lifetime.remaining -= deltaTime;

            if (lifetime.remaining <= 0.0f) {
                toDelete.push_back(entityId);
            }
        }

        // Perform deferred deletion
        for (auto entityId : toDelete) {
            ecs.entityDelete(entityId);
        }
    }

}  // namespace infrastructure::ecs::systems
