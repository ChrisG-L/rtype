/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LifetimeSystem - ECS System for timed entity expiration
*/

#ifndef LIFETIME_SYSTEM_HPP_
#define LIFETIME_SYSTEM_HPP_

#include "core/System.hpp"

namespace infrastructure::ecs::systems {

    /**
     * @brief System that manages timed entity lifetimes.
     *
     * Query: <LifetimeComp>
     * Priority: 600 (runs after gameplay logic)
     *
     * Decrements LifetimeComp::remaining by deltaTime.
     * When remaining <= 0, the entity is deleted.
     * Uses deferred deletion to avoid modifying collections during iteration.
     */
    class LifetimeSystem : public ECS::ISystem {
    public:
        LifetimeSystem() = default;
        ~LifetimeSystem() override = default;

        /**
         * @brief Update all entities with Lifetime components.
         *
         * @param ecs The ECS instance
         * @param thisID This system's ID (unused)
         * @param msecs Delta time in milliseconds
         */
        void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override;
    };

}  // namespace infrastructure::ecs::systems

#endif /* !LIFETIME_SYSTEM_HPP_ */
