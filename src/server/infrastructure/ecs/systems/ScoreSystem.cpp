/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ScoreSystem - Implementation
*/

#include "systems/ScoreSystem.hpp"
#include "core/ECS.hpp"
#include "components/PlayerTag.hpp"
#include "components/ScoreComp.hpp"

namespace infrastructure::ecs::systems {

    ScoreSystem::ScoreSystem(bridge::DomainBridge& bridge)
        : _bridge(bridge)
    {}

    void ScoreSystem::Update(ECS::ECS& ecs, [[maybe_unused]] ECS::SystemID thisID, uint32_t msecs) {
        float deltaTime = static_cast<float>(msecs) / 1000.0f;

        // Update combo decay for all players
        updateComboDecay(ecs, deltaTime);

        // Process kill events
        while (!_killQueue.empty()) {
            processKill(ecs, _killQueue.front());
            _killQueue.pop();
        }
    }

    void ScoreSystem::queueKill(const KillScoreEvent& event) {
        _killQueue.push(event);
    }

    void ScoreSystem::clearKills() {
        while (!_killQueue.empty()) {
            _killQueue.pop();
        }
    }

    std::vector<ScoreChangedEvent> ScoreSystem::getScoreChanges() {
        std::vector<ScoreChangedEvent> result = std::move(_scoreChanges);
        _scoreChanges.clear();
        return result;
    }

    void ScoreSystem::addBonusPoints(uint8_t playerId, uint16_t points) {
        // This will be handled in Update via a different mechanism
        // For now, store as a special kill event with 0 enemy type
        _killQueue.push({playerId, points, 0xFF});  // 0xFF = bonus points marker
    }

    void ScoreSystem::resetPlayerScore(uint8_t playerId) {
        // This needs to be called directly on the ECS
        // Can't access ECS from here, so this is a no-op placeholder
        // In practice, GameWorld will reset scores directly
        (void)playerId;
    }

    std::optional<ECS::EntityID> ScoreSystem::findPlayerByID(ECS::ECS& ecs, uint8_t playerId) {
        auto players = ecs.getEntitiesByComponentsAllOf<components::PlayerTag>();

        for (auto entityId : players) {
            const auto& tag = ecs.entityGetComponent<components::PlayerTag>(entityId);
            if (tag.playerId == playerId) {
                return entityId;
            }
        }
        return std::nullopt;
    }

    void ScoreSystem::updateComboDecay(ECS::ECS& ecs, float deltaTime) {
        auto players = ecs.getEntitiesByComponentsAllOf<
            components::PlayerTag,
            components::ScoreComp
        >();

        float graceTime = _bridge.getComboGraceTime();

        for (auto entityId : players) {
            auto& score = ecs.entityGetComponent<components::ScoreComp>(entityId);

            // Update combo timer
            score.comboTimer += deltaTime;

            // Only decay if past grace time and combo > 1.0
            if (score.comboTimer > graceTime && score.comboMultiplier > 1.0f) {
                float oldCombo = score.comboMultiplier;
                score.comboMultiplier = _bridge.decayCombo(
                    score.comboMultiplier,
                    deltaTime,
                    score.comboTimer
                );

                // Clamp to minimum of 1.0
                if (score.comboMultiplier < 1.0f) {
                    score.comboMultiplier = 1.0f;
                }

                // Track max combo achieved
                if (oldCombo > score.maxCombo) {
                    score.maxCombo = oldCombo;
                }
            }
        }
    }

    void ScoreSystem::processKill(ECS::ECS& ecs, const KillScoreEvent& event) {
        auto playerEntityOpt = findPlayerByID(ecs, event.playerId);
        if (!playerEntityOpt.has_value()) {
            return;
        }
        ECS::EntityID playerEntity = *playerEntityOpt;

        if (!ecs.entityHasComponent<components::ScoreComp>(playerEntity)) {
            return;
        }

        auto& score = ecs.entityGetComponent<components::ScoreComp>(playerEntity);

        // Check if this is a bonus points event (0xFF marker)
        bool isBonusPoints = (event.enemyType == 0xFF);

        uint32_t pointsToAdd;
        if (isBonusPoints) {
            // Bonus points don't get combo multiplier
            pointsToAdd = event.basePoints;
        } else {
            // Calculate score with combo multiplier
            pointsToAdd = _bridge.calculateKillScore(event.basePoints, score.comboMultiplier);

            // Increment combo
            score.comboMultiplier = _bridge.incrementCombo(score.comboMultiplier);

            // Track max combo
            if (score.comboMultiplier > score.maxCombo) {
                score.maxCombo = score.comboMultiplier;
            }

            // Increment kill count
            score.kills++;

            // Reset combo timer
            score.comboTimer = 0.0f;
        }

        // Add points to total
        score.total += pointsToAdd;

        // Record score change event
        _scoreChanges.push_back({
            event.playerId,
            score.total,
            static_cast<uint16_t>(pointsToAdd),
            score.comboMultiplier
        });
    }

}  // namespace infrastructure::ecs::systems
