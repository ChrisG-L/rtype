/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ScoreSystem - ECS System for score and combo management
*/

#ifndef SCORE_SYSTEM_HPP_
#define SCORE_SYSTEM_HPP_

#include "core/System.hpp"
#include "bridge/DomainBridge.hpp"
#include <cstdint>
#include <queue>
#include <functional>
#include <optional>

namespace infrastructure::ecs::systems {

    /**
     * @brief Event representing a kill that awards points.
     */
    struct KillScoreEvent {
        uint8_t playerId;       // Player who got the kill
        uint16_t basePoints;    // Base points for the enemy
        uint8_t enemyType;      // Type of enemy killed (for tracking)
    };

    /**
     * @brief Event for score changes (for UI/network broadcast).
     */
    struct ScoreChangedEvent {
        uint8_t playerId;
        uint32_t newTotal;
        uint16_t pointsAdded;
        float newCombo;
    };

    /**
     * @brief System that handles score calculation and combo decay.
     *
     * Query: <PlayerTag, ScoreComp>
     * Priority: 800 (after damage system)
     *
     * Decrements combo timer and decays combo.
     * Processes kill events to award points.
     * Uses DomainBridge for score calculations.
     */
    class ScoreSystem : public ECS::ISystem {
    public:
        /**
         * @brief Construct ScoreSystem with a DomainBridge.
         * @param bridge Reference to DomainBridge for score calculations
         */
        explicit ScoreSystem(bridge::DomainBridge& bridge);
        ~ScoreSystem() override = default;

        /**
         * @brief Update combo decay and process kill events.
         *
         * @param ecs The ECS instance
         * @param thisID This system's ID (unused)
         * @param msecs Delta time in milliseconds
         */
        void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override;

        /**
         * @brief Queue a kill event to award points.
         * @param event The kill score event
         */
        void queueKill(const KillScoreEvent& event);

        /**
         * @brief Clear all pending kill events.
         */
        void clearKills();

        /**
         * @brief Get score changed events since last call (for network broadcast).
         * @return Vector of score change events
         */
        std::vector<ScoreChangedEvent> getScoreChanges();

        /**
         * @brief Add points directly to a player (for bonus scores).
         * @param playerId Player ID
         * @param points Points to add (does not affect combo)
         */
        void addBonusPoints(uint8_t playerId, uint16_t points);

        /**
         * @brief Reset a player's score (for respawn/new game).
         * @param playerId Player ID
         */
        void resetPlayerScore(uint8_t playerId);

    private:
        bridge::DomainBridge& _bridge;

        std::queue<KillScoreEvent> _killQueue;
        std::vector<ScoreChangedEvent> _scoreChanges;

        /**
         * @brief Find player entity by player ID.
         * @return EntityID if found, std::nullopt otherwise
         */
        std::optional<ECS::EntityID> findPlayerByID(ECS::ECS& ecs, uint8_t playerId);

        /**
         * @brief Process a single kill event.
         */
        void processKill(ECS::ECS& ecs, const KillScoreEvent& event);

        /**
         * @brief Update combo decay for all players.
         */
        void updateComboDecay(ECS::ECS& ecs, float deltaTime);
    };

}  // namespace infrastructure::ecs::systems

#endif /* !SCORE_SYSTEM_HPP_ */
