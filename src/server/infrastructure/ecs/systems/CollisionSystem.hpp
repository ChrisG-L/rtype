/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** CollisionSystem - ECS System for collision detection
*/

#ifndef COLLISION_SYSTEM_HPP_
#define COLLISION_SYSTEM_HPP_

#include "core/System.hpp"
#include "bridge/DomainBridge.hpp"
#include <vector>

namespace infrastructure::ecs::systems {

    /**
     * @brief Represents a collision between two entities.
     */
    struct CollisionEvent {
        ECS::EntityID entityA;
        ECS::EntityID entityB;
        ECS::EntityGroup groupA;
        ECS::EntityGroup groupB;
    };

    /**
     * @brief System that detects AABB collisions between entities.
     *
     * Query: <PositionComp, HitboxComp>
     * Priority: 400 (runs after movement, before damage)
     *
     * OPTIMIZATION: Instead of checking ALL entities against ALL entities (O(n²)),
     * this system only checks relevant collision pairs:
     * - MISSILES vs ENEMIES
     * - WAVE_CANNONS vs ENEMIES
     * - PLAYERS vs ENEMY_MISSILES
     * - FORCE_PODS vs ENEMIES
     * - PLAYERS vs POWERUPS
     *
     * This reduces comparisons from n*(n-1)/2 to only meaningful pairs.
     * Entity groups are fetched once per frame, avoiding repeated lookups.
     */
    class CollisionSystem : public ECS::ISystem {
    public:
        /**
         * @brief Construct CollisionSystem with a DomainBridge.
         * @param bridge Reference to DomainBridge for collision checking
         */
        explicit CollisionSystem(bridge::DomainBridge& bridge);
        ~CollisionSystem() override = default;

        /**
         * @brief Detect all collisions this frame.
         *
         * @param ecs The ECS instance
         * @param thisID This system's ID (unused)
         * @param msecs Delta time in milliseconds (unused)
         */
        void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override;

        /**
         * @brief Get the list of collisions detected this frame.
         * @return Vector of collision events (cleared on next Update)
         */
        const std::vector<CollisionEvent>& getCollisions() const;

        /**
         * @brief Clear the collision list (called by DamageSystem after processing).
         */
        void clearCollisions();

    private:
        bridge::DomainBridge& _bridge;
        std::vector<CollisionEvent> _collisions;

        /**
         * @brief Check collisions between two groups of entities.
         *
         * @param ecs The ECS instance
         * @param groupA First group of entity IDs
         * @param groupB Second group of entity IDs
         * @param typeA EntityGroup type for groupA
         * @param typeB EntityGroup type for groupB
         */
        void checkPairs(ECS::ECS& ecs,
                        const std::vector<ECS::EntityID>& groupA,
                        const std::vector<ECS::EntityID>& groupB,
                        ECS::EntityGroup typeA,
                        ECS::EntityGroup typeB);
    };

}  // namespace infrastructure::ecs::systems

#endif /* !COLLISION_SYSTEM_HPP_ */
