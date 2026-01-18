/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** CleanupSystem - ECS System for removing out-of-bounds entities
*/

#ifndef CLEANUP_SYSTEM_HPP_
#define CLEANUP_SYSTEM_HPP_

#include "core/System.hpp"
#include "bridge/DomainBridge.hpp"

namespace infrastructure::ecs::systems {

    /**
     * @brief System that removes entities that have left the screen.
     *
     * Query: <PositionComp, HitboxComp>
     * Priority: 700 (runs after gameplay logic)
     *
     * Checks if entities are fully outside screen bounds using DomainBridge.
     * Does NOT delete PLAYERS (players are handled differently).
     * Uses deferred deletion to avoid modifying collections during iteration.
     */
    class CleanupSystem : public ECS::ISystem {
    public:
        /**
         * @brief Construct CleanupSystem with a DomainBridge.
         * @param bridge Reference to DomainBridge for bounds checking
         */
        explicit CleanupSystem(bridge::DomainBridge& bridge);
        ~CleanupSystem() override = default;

        /**
         * @brief Remove out-of-bounds entities.
         *
         * @param ecs The ECS instance
         * @param thisID This system's ID (unused)
         * @param msecs Delta time in milliseconds (unused)
         */
        void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override;

    private:
        bridge::DomainBridge& _bridge;
    };

}  // namespace infrastructure::ecs::systems

#endif /* !CLEANUP_SYSTEM_HPP_ */
