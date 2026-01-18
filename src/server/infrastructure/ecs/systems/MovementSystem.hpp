/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MovementSystem - ECS System for position updates based on velocity
*/

#ifndef MOVEMENT_SYSTEM_HPP_
#define MOVEMENT_SYSTEM_HPP_

#include "core/System.hpp"

namespace infrastructure::ecs::systems {

    /**
     * @brief System that updates entity positions based on their velocities.
     *
     * Query: <PositionComp, VelocityComp>
     * Priority: 300 (runs early in the update cycle)
     *
     * Applies simple physics: pos += vel × dt
     * Does NOT clamp positions to screen bounds (that's PlayerInputSystem's job).
     */
    class MovementSystem : public ECS::ISystem {
    public:
        MovementSystem() = default;
        ~MovementSystem() override = default;

        /**
         * @brief Update all entities with Position and Velocity components.
         *
         * @param ecs The ECS instance
         * @param thisID This system's ID (unused)
         * @param msecs Delta time in milliseconds
         */
        void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override;
    };

}  // namespace infrastructure::ecs::systems

#endif /* !MOVEMENT_SYSTEM_HPP_ */
