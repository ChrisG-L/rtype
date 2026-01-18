/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** DamageSystem - ECS System for applying damage from collisions
*/

#ifndef DAMAGE_SYSTEM_HPP_
#define DAMAGE_SYSTEM_HPP_

#include "core/System.hpp"
#include "bridge/DomainBridge.hpp"
#include "systems/CollisionSystem.hpp"
#include <vector>

namespace infrastructure::ecs::systems {

    /**
     * @brief Represents a kill event for score tracking.
     */
    struct KillEvent {
        ECS::EntityID killerEntity;  // Entity that caused the kill
        ECS::EntityID killedEntity;  // Entity that was killed
        uint8_t killerPlayerId;      // Player ID who gets the points (from OwnerComp)
        uint8_t killedType;          // EnemyTag::type or similar
        uint16_t basePoints;         // Base score value
    };

    /**
     * @brief System that processes collisions and applies damage.
     *
     * Query: Uses CollisionSystem events + HealthComp
     * Priority: 500 (runs after CollisionSystem)
     *
     * Collision damage rules:
     * - MISSILES + ENEMIES → Enemy takes missile damage
     * - PLAYERS + ENEMIES → Player takes contact damage
     * - PLAYERS + ENEMY_MISSILES → Player takes missile damage
     * - MISSILES + MISSILES → Ignored (no friendly fire)
     * - WAVE_CANNONS + ENEMIES → Enemy takes wave cannon damage
     * - FORCE_PODS + ENEMIES → Enemy takes contact damage
     *
     * Respects invulnerability (HealthComp::invulnerable).
     */
    class DamageSystem : public ECS::ISystem {
    public:
        /**
         * @brief Construct DamageSystem with dependencies.
         * @param bridge Reference to DomainBridge for damage calculations
         * @param collisionSystem Reference to CollisionSystem for events
         */
        DamageSystem(bridge::DomainBridge& bridge, CollisionSystem& collisionSystem);
        ~DamageSystem() override = default;

        /**
         * @brief Process collisions and apply damage.
         *
         * @param ecs The ECS instance
         * @param thisID This system's ID (unused)
         * @param msecs Delta time in milliseconds (unused)
         */
        void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override;

        /**
         * @brief Get kill events from this frame (for ScoreSystem).
         * @return Vector of kill events (cleared on next Update)
         */
        const std::vector<KillEvent>& getKillEvents() const;

        /**
         * @brief Clear kill events (called by ScoreSystem after processing).
         */
        void clearKillEvents();

    private:
        bridge::DomainBridge& _bridge;
        CollisionSystem& _collisionSystem;
        std::vector<KillEvent> _killEvents;

        /**
         * @brief Process a collision between two entities.
         */
        void processCollision(ECS::ECS& ecs, const CollisionEvent& collision);

        /**
         * @brief Get missile damage from MissileTag component.
         */
        uint8_t getMissileDamage(ECS::ECS& ecs, ECS::EntityID missile);

        /**
         * @brief Get player ID from OwnerComp.
         */
        uint8_t getOwnerPlayerId(ECS::ECS& ecs, ECS::EntityID entity);

        /**
         * @brief Check if entity is invulnerable.
         */
        bool isInvulnerable(ECS::ECS& ecs, ECS::EntityID entity);
    };

}  // namespace infrastructure::ecs::systems

#endif /* !DAMAGE_SYSTEM_HPP_ */
