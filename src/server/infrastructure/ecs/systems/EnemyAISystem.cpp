/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** EnemyAISystem - Implementation
*/

#include "systems/EnemyAISystem.hpp"
#include "core/ECS.hpp"
#include "components/EnemyTag.hpp"
#include "components/EnemyAIComp.hpp"
#include "components/PositionComp.hpp"
#include "components/VelocityComp.hpp"
#include "components/PlayerTag.hpp"
#include "domain/services/EnemyBehavior.hpp"
#include <cmath>
#include <limits>

namespace infrastructure::ecs::systems {

    // Default screen center for tracker fallback
    constexpr float DEFAULT_TARGET_Y = 300.0f;

    // Enemy missile spawn offset
    constexpr float MISSILE_SPAWN_OFFSET_X = -30.0f;  // Spawn in front (left) of enemy

    EnemyAISystem::EnemyAISystem(bridge::DomainBridge& bridge)
        : _bridge(bridge)
    {}

    void EnemyAISystem::Update(ECS::ECS& ecs, [[maybe_unused]] ECS::SystemID thisID, uint32_t msecs) {
        float deltaTime = static_cast<float>(msecs) / 1000.0f;

        // Update player positions for tracker targeting
        _playerPositions.clear();
        auto players = ecs.getEntitiesByComponentsAllOf<components::PlayerTag, components::PositionComp>();
        for (auto playerEntity : players) {
            const auto& pos = ecs.entityGetComponent<components::PositionComp>(playerEntity);
            _playerPositions.push_back({0, pos.x, pos.y});  // playerId not needed for targeting
        }

        // Get all enemies
        auto enemies = ecs.getEntitiesByComponentsAllOf<
            components::EnemyTag,
            components::EnemyAIComp,
            components::PositionComp
        >();

        for (auto entityId : enemies) {
            // Update movement
            updateEnemyMovement(ecs, entityId, deltaTime);

            // Update shooting
            updateEnemyShooting(ecs, entityId, deltaTime);
        }
    }

    std::vector<EnemyMissileRequest> EnemyAISystem::getMissileRequests() {
        std::vector<EnemyMissileRequest> result = std::move(_missileRequests);
        _missileRequests.clear();
        return result;
    }

    void EnemyAISystem::clearMissileRequests() {
        _missileRequests.clear();
    }

    void EnemyAISystem::setPlayerTargetY(uint8_t playerId, float y) {
        for (auto& pp : _playerPositions) {
            if (pp.playerId == playerId) {
                pp.y = y;
                return;
            }
        }
        // Not found, add new
        _playerPositions.push_back({playerId, 0.0f, y});
    }

    float EnemyAISystem::getNearestPlayerY(float enemyX, float enemyY) const {
        if (_playerPositions.empty()) {
            return DEFAULT_TARGET_Y;
        }

        float nearestY = DEFAULT_TARGET_Y;
        float minDist = std::numeric_limits<float>::max();

        for (const auto& pp : _playerPositions) {
            float dx = pp.x - enemyX;
            float dy = pp.y - enemyY;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist < minDist) {
                minDist = dist;
                nearestY = pp.y;
            }
        }

        return nearestY;
    }

    void EnemyAISystem::updateEnemyMovement(ECS::ECS& ecs, ECS::EntityID entityId, float deltaTime) {
        const auto& tag = ecs.entityGetComponent<components::EnemyTag>(entityId);
        auto& ai = ecs.entityGetComponent<components::EnemyAIComp>(entityId);
        auto& pos = ecs.entityGetComponent<components::PositionComp>(entityId);

        // Update alive time
        ai.aliveTime += deltaTime;

        // Get target Y for tracker enemies
        float targetY = getNearestPlayerY(pos.x, pos.y);
        ai.targetY = targetY;  // Store for potential use

        // Create zigzag state from component
        domain::services::ZigzagState zigzag;
        zigzag.timer = ai.zigzagTimer;
        zigzag.goingUp = ai.zigzagUp;

        // Get movement from DomainBridge
        auto [dx, dy] = _bridge.getEnemyMovement(
            tag.type,
            pos.y,
            ai.baseY,
            ai.aliveTime,
            ai.phaseOffset,
            deltaTime,
            targetY,
            zigzag
        );

        // Update zigzag state back to component
        ai.zigzagTimer = zigzag.timer;
        ai.zigzagUp = zigzag.goingUp;

        // Apply movement - set velocity for MovementSystem
        if (ecs.entityHasComponent<components::VelocityComp>(entityId)) {
            auto& vel = ecs.entityGetComponent<components::VelocityComp>(entityId);
            // dx/dy from DomainBridge are deltas per frame, convert to velocity
            // Since MovementSystem does pos += vel * dt, we need vel = delta / dt
            if (deltaTime > 0.0f) {
                vel.x = dx / deltaTime;
                vel.y = dy / deltaTime;
            }
        } else {
            // No velocity component, apply position directly
            pos.x += dx;
            pos.y += dy;
        }
    }

    void EnemyAISystem::updateEnemyShooting(ECS::ECS& ecs, ECS::EntityID entityId, float deltaTime) {
        const auto& tag = ecs.entityGetComponent<components::EnemyTag>(entityId);
        auto& ai = ecs.entityGetComponent<components::EnemyAIComp>(entityId);
        const auto& pos = ecs.entityGetComponent<components::PositionComp>(entityId);

        // Decrement shoot cooldown
        ai.shootCooldown -= deltaTime;

        // Check if can shoot
        if (_bridge.canEnemyShoot(ai.shootCooldown)) {
            // Spawn missile request
            _missileRequests.push_back({
                pos.x + MISSILE_SPAWN_OFFSET_X,
                pos.y,
                tag.type
            });

            // Reset cooldown
            ai.shootCooldown = _bridge.getEnemyShootInterval(tag.type);
        }
    }

}  // namespace infrastructure::ecs::systems
