/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** EnemyAISystem - ECS System for enemy AI movement and shooting
*/

#ifndef ENEMY_AI_SYSTEM_HPP_
#define ENEMY_AI_SYSTEM_HPP_

#include "core/System.hpp"
#include "bridge/DomainBridge.hpp"
#include <cstdint>
#include <vector>
#include <functional>

namespace infrastructure::ecs::systems {

    /**
     * @brief Request to spawn an enemy missile.
     */
    struct EnemyMissileRequest {
        float x, y;             // Spawn position
        uint8_t enemyType;      // Type of enemy shooting
    };

    /**
     * @brief System that handles enemy AI movement and shooting.
     *
     * Query: <EnemyTag, EnemyAIComp, PositionComp, VelocityComp>
     * Priority: 100 (after input, before movement)
     *
     * Updates enemy movement patterns based on type.
     * Handles enemy shooting cooldowns.
     * Uses DomainBridge for movement calculations.
     */
    class EnemyAISystem : public ECS::ISystem {
    public:
        /**
         * @brief Construct EnemyAISystem with a DomainBridge.
         * @param bridge Reference to DomainBridge for AI calculations
         */
        explicit EnemyAISystem(bridge::DomainBridge& bridge);
        ~EnemyAISystem() override = default;

        /**
         * @brief Update enemy AI (movement and shooting).
         *
         * @param ecs The ECS instance
         * @param thisID This system's ID (unused)
         * @param msecs Delta time in milliseconds
         */
        void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override;

        /**
         * @brief Get pending missile spawn requests (for WeaponSystem or GameWorld).
         * @return Vector of missile requests
         */
        std::vector<EnemyMissileRequest> getMissileRequests();

        /**
         * @brief Clear pending missile requests.
         */
        void clearMissileRequests();

        /**
         * @brief Set the target Y position for tracker enemies.
         *
         * This should be called each frame with the nearest player's Y.
         * @param playerId Player ID
         * @param y Player Y position
         */
        void setPlayerTargetY(uint8_t playerId, float y);

        /**
         * @brief Get nearest player Y for a given enemy position.
         * @return Nearest player Y or screen center if no players
         */
        float getNearestPlayerY(float enemyX, float enemyY) const;

    private:
        bridge::DomainBridge& _bridge;

        std::vector<EnemyMissileRequest> _missileRequests;

        // Player positions for tracker targeting
        struct PlayerPosition {
            uint8_t playerId;
            float x, y;
        };
        std::vector<PlayerPosition> _playerPositions;

        /**
         * @brief Update movement for a single enemy.
         */
        void updateEnemyMovement(ECS::ECS& ecs, ECS::EntityID entityId, float deltaTime);

        /**
         * @brief Update shooting for a single enemy.
         */
        void updateEnemyShooting(ECS::ECS& ecs, ECS::EntityID entityId, float deltaTime);
    };

}  // namespace infrastructure::ecs::systems

#endif /* !ENEMY_AI_SYSTEM_HPP_ */
