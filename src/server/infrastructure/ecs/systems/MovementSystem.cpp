/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MovementSystem - Implementation
*/

#include "systems/MovementSystem.hpp"
#include "core/ECS.hpp"
#include "components/PositionComp.hpp"
#include "components/VelocityComp.hpp"

namespace infrastructure::ecs::systems {

    void MovementSystem::Update(ECS::ECS& ecs, [[maybe_unused]] ECS::SystemID thisID, uint32_t msecs) {
        // Convert milliseconds to seconds for physics calculations
        float deltaTime = static_cast<float>(msecs) / 1000.0f;

        // Query all entities with both Position and Velocity components
        auto entities = ecs.getEntitiesByComponentsAllOf<
            components::PositionComp,
            components::VelocityComp
        >();

        // Update positions based on velocity
        for (auto entityId : entities) {
            auto& pos = ecs.entityGetComponent<components::PositionComp>(entityId);
            const auto& vel = ecs.entityGetComponent<components::VelocityComp>(entityId);

            pos.x += vel.x * deltaTime;
            pos.y += vel.y * deltaTime;
        }
    }

}  // namespace infrastructure::ecs::systems
