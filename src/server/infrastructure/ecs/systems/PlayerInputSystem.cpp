/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** PlayerInputSystem - Implementation
*/

#include "systems/PlayerInputSystem.hpp"
#include "core/ECS.hpp"
#include "components/PlayerTag.hpp"
#include "components/PositionComp.hpp"
#include "components/VelocityComp.hpp"
#include "components/SpeedLevelComp.hpp"
#include "components/HitboxComp.hpp"

namespace infrastructure::ecs::systems {

    PlayerInputSystem::PlayerInputSystem(bridge::DomainBridge& bridge)
        : _bridge(bridge)
    {}

    void PlayerInputSystem::Update(ECS::ECS& ecs, [[maybe_unused]] ECS::SystemID thisID, [[maybe_unused]] uint32_t msecs) {
        // Process all pending inputs
        while (!_inputQueue.empty()) {
            processInput(ecs, _inputQueue.front());
            _inputQueue.pop();
        }

        // Clamp all player positions to screen bounds
        if (_clampEnabled) {
            auto players = ecs.getEntitiesByComponentsAllOf<
                components::PlayerTag,
                components::PositionComp
            >();

            for (auto entityId : players) {
                auto& pos = ecs.entityGetComponent<components::PositionComp>(entityId);

                // Get hitbox dimensions if available
                float width = 50.0f;   // Default player width
                float height = 30.0f;  // Default player height
                if (ecs.entityHasComponent<components::HitboxComp>(entityId)) {
                    const auto& hitbox = ecs.entityGetComponent<components::HitboxComp>(entityId);
                    width = hitbox.width;
                    height = hitbox.height;
                }

                _bridge.clampToScreen(pos.x, pos.y, width, height);
            }
        }
    }

    void PlayerInputSystem::queueInput(const PlayerInputEvent& event) {
        _inputQueue.push(event);
    }

    void PlayerInputSystem::clearInputs() {
        while (!_inputQueue.empty()) {
            _inputQueue.pop();
        }
    }

    void PlayerInputSystem::setClampEnabled(bool enabled) {
        _clampEnabled = enabled;
    }

    std::optional<ECS::EntityID> PlayerInputSystem::findPlayerByID(ECS::ECS& ecs, uint8_t playerId) {
        auto players = ecs.getEntitiesByComponentsAllOf<components::PlayerTag>();

        for (auto entityId : players) {
            const auto& tag = ecs.entityGetComponent<components::PlayerTag>(entityId);
            if (tag.playerId == playerId) {
                return entityId;
            }
        }
        return std::nullopt;
    }

    void PlayerInputSystem::processInput(ECS::ECS& ecs, const PlayerInputEvent& event) {
        auto playerEntityOpt = findPlayerByID(ecs, event.playerId);
        if (!playerEntityOpt.has_value()) {
            return;  // Player not found
        }
        ECS::EntityID playerEntity = *playerEntityOpt;

        // Check if player has required components
        if (!ecs.entityHasComponent<components::VelocityComp>(playerEntity)) {
            return;
        }

        auto& vel = ecs.entityGetComponent<components::VelocityComp>(playerEntity);

        // Get speed multiplier from SpeedLevelComp
        uint8_t speedLevel = 0;
        if (ecs.entityHasComponent<components::SpeedLevelComp>(playerEntity)) {
            speedLevel = ecs.entityGetComponent<components::SpeedLevelComp>(playerEntity).level;
        }

        float baseSpeed = _bridge.getPlayerSpeed(speedLevel);

        // Reset velocity
        vel.x = 0.0f;
        vel.y = 0.0f;

        // Apply input keys to velocity
        if (event.keys & InputKeys::UP) {
            vel.y = -baseSpeed;
        }
        if (event.keys & InputKeys::DOWN) {
            vel.y = baseSpeed;
        }
        if (event.keys & InputKeys::LEFT) {
            vel.x = -baseSpeed;
        }
        if (event.keys & InputKeys::RIGHT) {
            vel.x = baseSpeed;
        }

        // Normalize diagonal movement
        if (vel.x != 0.0f && vel.y != 0.0f) {
            constexpr float DIAGONAL_FACTOR = 0.7071f;  // 1/sqrt(2)
            vel.x *= DIAGONAL_FACTOR;
            vel.y *= DIAGONAL_FACTOR;
        }
    }

}  // namespace infrastructure::ecs::systems
