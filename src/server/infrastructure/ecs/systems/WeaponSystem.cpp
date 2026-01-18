/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** WeaponSystem - Implementation
*/

#include "systems/WeaponSystem.hpp"
#include "core/ECS.hpp"
#include "components/PlayerTag.hpp"
#include "components/PositionComp.hpp"
#include "components/VelocityComp.hpp"
#include "components/WeaponComp.hpp"
#include "components/MissileTag.hpp"
#include "components/HitboxComp.hpp"
#include "components/OwnerComp.hpp"
#include "components/LifetimeComp.hpp"
#include "components/WaveCannonTag.hpp"
#include "Protocol.hpp"
#include <cmath>

namespace infrastructure::ecs::systems {

    // Missile spawn offset from player position
    constexpr float MISSILE_SPAWN_OFFSET_X = 50.0f;  // Spawn in front of ship
    constexpr float MISSILE_SPAWN_OFFSET_Y = 0.0f;

    // Hitbox sizes
    constexpr float MISSILE_WIDTH = 16.0f;
    constexpr float MISSILE_HEIGHT = 8.0f;
    constexpr float WAVE_CANNON_HEIGHT = 20.0f;  // Base height, varies by level

    // Spread weapon angle
    constexpr float SPREAD_ANGLE_DEG = 15.0f;
    constexpr float DEG_TO_RAD = 3.14159265f / 180.0f;

    WeaponSystem::WeaponSystem(bridge::DomainBridge& bridge)
        : _bridge(bridge)
    {
        // Default missile ID generator
        _missileIdGenerator = [this]() {
            return _nextMissileId++;
        };
    }

    void WeaponSystem::Update(ECS::ECS& ecs, [[maybe_unused]] ECS::SystemID thisID, uint32_t msecs) {
        float deltaTime = static_cast<float>(msecs) / 1000.0f;

        // Update cooldowns
        updateCooldowns(ecs, deltaTime);

        // Update charge times
        updateChargeTimes(ecs, deltaTime);

        // Process weapon switch requests
        while (!_switchQueue.empty()) {
            processSwitchWeapon(ecs, _switchQueue.front());
            _switchQueue.pop();
        }

        // Process charge start requests
        while (!_chargeStartQueue.empty()) {
            processChargeStart(ecs, _chargeStartQueue.front());
            _chargeStartQueue.pop();
        }

        // Process charge release requests
        while (!_chargeReleaseQueue.empty()) {
            processChargeRelease(ecs, _chargeReleaseQueue.front());
            _chargeReleaseQueue.pop();
        }

        // Process shoot requests
        while (!_shootQueue.empty()) {
            processShoot(ecs, _shootQueue.front());
            _shootQueue.pop();
        }
    }

    void WeaponSystem::queueShoot(const ShootRequest& request) {
        _shootQueue.push(request);
    }

    void WeaponSystem::queueChargeStart(const ChargeStartRequest& request) {
        _chargeStartQueue.push(request);
    }

    void WeaponSystem::queueChargeRelease(const ChargeReleaseRequest& request) {
        _chargeReleaseQueue.push(request);
    }

    void WeaponSystem::queueSwitchWeapon(const SwitchWeaponRequest& request) {
        _switchQueue.push(request);
    }

    void WeaponSystem::clearRequests() {
        while (!_shootQueue.empty()) _shootQueue.pop();
        while (!_chargeStartQueue.empty()) _chargeStartQueue.pop();
        while (!_chargeReleaseQueue.empty()) _chargeReleaseQueue.pop();
        while (!_switchQueue.empty()) _switchQueue.pop();
    }

    std::vector<MissileSpawnedEvent> WeaponSystem::getSpawnedMissiles() {
        std::vector<MissileSpawnedEvent> result = std::move(_spawnedMissiles);
        _spawnedMissiles.clear();
        return result;
    }

    void WeaponSystem::setMissileIdGenerator(std::function<uint16_t()> generator) {
        _missileIdGenerator = std::move(generator);
    }

    std::optional<ECS::EntityID> WeaponSystem::findPlayerByID(ECS::ECS& ecs, uint8_t playerId) {
        auto players = ecs.getEntitiesByComponentsAllOf<components::PlayerTag>();

        for (auto entityId : players) {
            const auto& tag = ecs.entityGetComponent<components::PlayerTag>(entityId);
            if (tag.playerId == playerId) {
                return entityId;
            }
        }
        return std::nullopt;
    }

    void WeaponSystem::updateCooldowns(ECS::ECS& ecs, float deltaTime) {
        auto players = ecs.getEntitiesByComponentsAllOf<
            components::PlayerTag,
            components::WeaponComp
        >();

        for (auto entityId : players) {
            auto& weapon = ecs.entityGetComponent<components::WeaponComp>(entityId);
            if (weapon.shootCooldown > 0.0f) {
                weapon.shootCooldown -= deltaTime;
                if (weapon.shootCooldown < 0.0f) {
                    weapon.shootCooldown = 0.0f;
                }
            }
        }
    }

    void WeaponSystem::updateChargeTimes(ECS::ECS& ecs, float deltaTime) {
        auto players = ecs.getEntitiesByComponentsAllOf<
            components::PlayerTag,
            components::WeaponComp
        >();

        for (auto entityId : players) {
            auto& weapon = ecs.entityGetComponent<components::WeaponComp>(entityId);
            if (weapon.isCharging) {
                weapon.chargeTime += deltaTime;
            }
        }
    }

    void WeaponSystem::processShoot(ECS::ECS& ecs, const ShootRequest& request) {
        auto playerEntityOpt = findPlayerByID(ecs, request.playerId);
        if (!playerEntityOpt.has_value()) {
            return;
        }
        ECS::EntityID playerEntity = *playerEntityOpt;

        // Check required components
        if (!ecs.entityHasComponent<components::WeaponComp>(playerEntity) ||
            !ecs.entityHasComponent<components::PositionComp>(playerEntity)) {
            return;
        }

        auto& weapon = ecs.entityGetComponent<components::WeaponComp>(playerEntity);
        const auto& pos = ecs.entityGetComponent<components::PositionComp>(playerEntity);

        // Check if player is charging (can't shoot while charging)
        if (weapon.isCharging) {
            return;
        }

        // Check cooldown
        if (weapon.shootCooldown > 0.0f) {
            return;
        }

        uint8_t weaponType = weapon.currentType;
        uint8_t weaponLevel = weapon.getCurrentLevel();

        // Spawn missile(s) based on weapon type
        float spawnX = pos.x + MISSILE_SPAWN_OFFSET_X;
        float spawnY = pos.y + MISSILE_SPAWN_OFFSET_Y;

        if (weaponType == static_cast<uint8_t>(WeaponType::Spread)) {
            // Spread shot: 3 missiles with angles
            float angleRad = SPREAD_ANGLE_DEG * DEG_TO_RAD;

            // Center missile
            spawnMissile(ecs, request.playerId, spawnX, spawnY, weaponType, weaponLevel);

            // Upper missile (angled up)
            auto upperEntity = spawnMissile(ecs, request.playerId, spawnX, spawnY - 5.0f, weaponType, weaponLevel);
            if (upperEntity != 0 && ecs.entityHasComponent<components::VelocityComp>(upperEntity)) {
                auto& vel = ecs.entityGetComponent<components::VelocityComp>(upperEntity);
                float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
                vel.x = speed * std::cos(-angleRad);
                vel.y = speed * std::sin(-angleRad);
            }

            // Lower missile (angled down)
            auto lowerEntity = spawnMissile(ecs, request.playerId, spawnX, spawnY + 5.0f, weaponType, weaponLevel);
            if (lowerEntity != 0 && ecs.entityHasComponent<components::VelocityComp>(lowerEntity)) {
                auto& vel = ecs.entityGetComponent<components::VelocityComp>(lowerEntity);
                float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
                vel.x = speed * std::cos(angleRad);
                vel.y = speed * std::sin(angleRad);
            }
        } else {
            // Standard, Laser, Missile: single shot
            spawnMissile(ecs, request.playerId, spawnX, spawnY, weaponType, weaponLevel);
        }

        // Reset cooldown
        weapon.shootCooldown = _bridge.getWeaponCooldown(weaponType, weaponLevel);
    }

    void WeaponSystem::processChargeStart(ECS::ECS& ecs, const ChargeStartRequest& request) {
        auto playerEntityOpt = findPlayerByID(ecs, request.playerId);
        if (!playerEntityOpt.has_value()) {
            return;
        }
        ECS::EntityID playerEntity = *playerEntityOpt;

        if (!ecs.entityHasComponent<components::WeaponComp>(playerEntity)) {
            return;
        }

        auto& weapon = ecs.entityGetComponent<components::WeaponComp>(playerEntity);

        // Start charging
        weapon.isCharging = true;
        weapon.chargeTime = 0.0f;
    }

    void WeaponSystem::processChargeRelease(ECS::ECS& ecs, const ChargeReleaseRequest& request) {
        auto playerEntityOpt = findPlayerByID(ecs, request.playerId);
        if (!playerEntityOpt.has_value()) {
            return;
        }
        ECS::EntityID playerEntity = *playerEntityOpt;

        if (!ecs.entityHasComponent<components::WeaponComp>(playerEntity) ||
            !ecs.entityHasComponent<components::PositionComp>(playerEntity)) {
            return;
        }

        auto& weapon = ecs.entityGetComponent<components::WeaponComp>(playerEntity);
        const auto& pos = ecs.entityGetComponent<components::PositionComp>(playerEntity);

        if (!weapon.isCharging) {
            return;
        }

        // Determine charge level based on charge time
        uint8_t chargeLevel = 0;
        if (weapon.chargeTime >= WaveCannon::CHARGE_TIME_LV3) {
            chargeLevel = 3;
        } else if (weapon.chargeTime >= WaveCannon::CHARGE_TIME_LV2) {
            chargeLevel = 2;
        } else if (weapon.chargeTime >= WaveCannon::CHARGE_TIME_LV1) {
            chargeLevel = 1;
        }

        // Only spawn if charged enough
        if (chargeLevel > 0) {
            float spawnX = pos.x + MISSILE_SPAWN_OFFSET_X;
            float spawnY = pos.y;
            spawnWaveCannon(ecs, request.playerId, spawnX, spawnY, chargeLevel);
        }

        // Reset charging state
        weapon.isCharging = false;
        weapon.chargeTime = 0.0f;
    }

    void WeaponSystem::processSwitchWeapon(ECS::ECS& ecs, const SwitchWeaponRequest& request) {
        auto playerEntityOpt = findPlayerByID(ecs, request.playerId);
        if (!playerEntityOpt.has_value()) {
            return;
        }
        ECS::EntityID playerEntity = *playerEntityOpt;

        if (!ecs.entityHasComponent<components::WeaponComp>(playerEntity)) {
            return;
        }

        auto& weapon = ecs.entityGetComponent<components::WeaponComp>(playerEntity);

        // Calculate new weapon type
        int newType = static_cast<int>(weapon.currentType) + request.direction;

        // Wrap around (0-3 for Standard, Spread, Laser, Missile)
        if (newType < 0) {
            newType = MAX_SELECTABLE_WEAPONS - 1;
        } else if (newType >= MAX_SELECTABLE_WEAPONS) {
            newType = 0;
        }

        weapon.currentType = static_cast<uint8_t>(newType);
    }

    ECS::EntityID WeaponSystem::spawnMissile(ECS::ECS& ecs, uint8_t ownerId, float x, float y,
                                              uint8_t weaponType, uint8_t weaponLevel) {
        auto entity = ecs.entityCreate(ECS::EntityGroup::MISSILES);

        // Position
        auto& pos = ecs.entityAddComponent<components::PositionComp>(entity);
        pos.x = x;
        pos.y = y;

        // Velocity
        float speed = _bridge.getMissileSpeed(weaponType, weaponLevel);
        auto& vel = ecs.entityAddComponent<components::VelocityComp>(entity);
        vel.x = speed;  // Missiles go right
        vel.y = 0.0f;

        // Missile tag
        auto& tag = ecs.entityAddComponent<components::MissileTag>(entity);
        tag.weaponType = weaponType;
        tag.baseDamage = _bridge.getMissileDamage(weaponType, weaponLevel);
        tag.isHoming = (weaponType == static_cast<uint8_t>(WeaponType::Missile));
        tag.targetId = 0;

        // Hitbox
        auto& hitbox = ecs.entityAddComponent<components::HitboxComp>(entity);
        hitbox.width = MISSILE_WIDTH;
        hitbox.height = MISSILE_HEIGHT;
        hitbox.offsetX = 0.0f;
        hitbox.offsetY = 0.0f;

        // Owner
        auto& owner = ecs.entityAddComponent<components::OwnerComp>(entity);
        owner.ownerId = ownerId;
        owner.isPlayerOwned = true;

        // Lifetime (missiles live for ~10 seconds max)
        auto& lifetime = ecs.entityAddComponent<components::LifetimeComp>(entity);
        lifetime.remaining = 10.0f;

        // Record spawn event
        uint16_t missileId = generateMissileId();
        _spawnedMissiles.push_back({missileId, ownerId, x, y, weaponType});

        return entity;
    }

    ECS::EntityID WeaponSystem::spawnWaveCannon(ECS::ECS& ecs, uint8_t ownerId, float x, float y,
                                                 uint8_t chargeLevel) {
        auto entity = ecs.entityCreate(ECS::EntityGroup::WAVE_CANNONS);

        // Position
        auto& pos = ecs.entityAddComponent<components::PositionComp>(entity);
        pos.x = x;
        pos.y = y;

        // Velocity
        auto& vel = ecs.entityAddComponent<components::VelocityComp>(entity);
        vel.x = WaveCannon::SPEED;
        vel.y = 0.0f;

        // Wave Cannon tag
        auto& tag = ecs.entityAddComponent<components::WaveCannonTag>(entity);
        tag.chargeLevel = chargeLevel;
        switch (chargeLevel) {
            case 1:
                tag.width = WaveCannon::WIDTH_LV1;
                break;
            case 2:
                tag.width = WaveCannon::WIDTH_LV2;
                break;
            case 3:
            default:
                tag.width = WaveCannon::WIDTH_LV3;
                break;
        }

        // Hitbox (width varies by charge level)
        auto& hitbox = ecs.entityAddComponent<components::HitboxComp>(entity);
        hitbox.width = 100.0f;  // Beam is long
        hitbox.height = tag.width;  // Height based on charge level
        hitbox.offsetX = 0.0f;
        hitbox.offsetY = -tag.width / 2.0f;  // Center the hitbox

        // Owner
        auto& owner = ecs.entityAddComponent<components::OwnerComp>(entity);
        owner.ownerId = ownerId;
        owner.isPlayerOwned = true;

        // Lifetime (Wave Cannon travels fast, short lifetime)
        auto& lifetime = ecs.entityAddComponent<components::LifetimeComp>(entity);
        lifetime.remaining = 2.0f;

        return entity;
    }

    uint16_t WeaponSystem::generateMissileId() {
        if (_missileIdGenerator) {
            return _missileIdGenerator();
        }
        return _nextMissileId++;
    }

}  // namespace infrastructure::ecs::systems
