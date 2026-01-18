/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** WeaponSystem - ECS System for weapon cooldowns and missile spawning
*/

#ifndef WEAPON_SYSTEM_HPP_
#define WEAPON_SYSTEM_HPP_

#include "core/System.hpp"
#include "bridge/DomainBridge.hpp"
#include <cstdint>
#include <queue>
#include <functional>
#include <optional>

namespace infrastructure::ecs::systems {

    /**
     * @brief Request to shoot a weapon.
     */
    struct ShootRequest {
        uint8_t playerId;       // Player ID (1-4)
        uint16_t sequenceNum;   // For client-side prediction
    };

    /**
     * @brief Request to start charging Wave Cannon.
     */
    struct ChargeStartRequest {
        uint8_t playerId;
    };

    /**
     * @brief Request to release charged Wave Cannon.
     */
    struct ChargeReleaseRequest {
        uint8_t playerId;
        uint8_t chargeLevel;    // 1-3
    };

    /**
     * @brief Request to switch weapon.
     */
    struct SwitchWeaponRequest {
        uint8_t playerId;
        int8_t direction;       // -1 = previous, +1 = next
    };

    /**
     * @brief Information about a spawned missile (for network broadcast).
     */
    struct MissileSpawnedEvent {
        uint16_t missileId;
        uint8_t ownerId;
        float x, y;
        uint8_t weaponType;
    };

    /**
     * @brief System that handles weapon cooldowns and missile spawning.
     *
     * Query: <PlayerTag, WeaponComp, PositionComp>
     * Priority: 200 (after input, before collision)
     *
     * Decrements weapon cooldowns.
     * Processes shoot requests to spawn missiles.
     * Handles Wave Cannon charging.
     * Uses DomainBridge for weapon stats.
     */
    class WeaponSystem : public ECS::ISystem {
    public:
        /**
         * @brief Construct WeaponSystem with a DomainBridge.
         * @param bridge Reference to DomainBridge for weapon calculations
         */
        explicit WeaponSystem(bridge::DomainBridge& bridge);
        ~WeaponSystem() override = default;

        /**
         * @brief Update weapon cooldowns and process shoot requests.
         *
         * @param ecs The ECS instance
         * @param thisID This system's ID (unused)
         * @param msecs Delta time in milliseconds
         */
        void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override;

        /**
         * @brief Queue a shoot request.
         * @param request The shoot request
         */
        void queueShoot(const ShootRequest& request);

        /**
         * @brief Queue a charge start request.
         */
        void queueChargeStart(const ChargeStartRequest& request);

        /**
         * @brief Queue a charge release request.
         */
        void queueChargeRelease(const ChargeReleaseRequest& request);

        /**
         * @brief Queue a weapon switch request.
         */
        void queueSwitchWeapon(const SwitchWeaponRequest& request);

        /**
         * @brief Clear all pending requests.
         */
        void clearRequests();

        /**
         * @brief Get spawned missiles since last call (for network broadcast).
         * @return Vector of missile spawn events
         */
        std::vector<MissileSpawnedEvent> getSpawnedMissiles();

        /**
         * @brief Set callback for missile ID generation.
         * @param generator Function that returns unique missile IDs
         */
        void setMissileIdGenerator(std::function<uint16_t()> generator);

    private:
        bridge::DomainBridge& _bridge;

        std::queue<ShootRequest> _shootQueue;
        std::queue<ChargeStartRequest> _chargeStartQueue;
        std::queue<ChargeReleaseRequest> _chargeReleaseQueue;
        std::queue<SwitchWeaponRequest> _switchQueue;

        std::vector<MissileSpawnedEvent> _spawnedMissiles;
        std::function<uint16_t()> _missileIdGenerator;
        uint16_t _nextMissileId = 1;

        /**
         * @brief Find player entity by player ID.
         * @return EntityID if found, std::nullopt otherwise
         */
        std::optional<ECS::EntityID> findPlayerByID(ECS::ECS& ecs, uint8_t playerId);

        /**
         * @brief Process a single shoot request.
         */
        void processShoot(ECS::ECS& ecs, const ShootRequest& request);

        /**
         * @brief Process a charge start request.
         */
        void processChargeStart(ECS::ECS& ecs, const ChargeStartRequest& request);

        /**
         * @brief Process a charge release request.
         */
        void processChargeRelease(ECS::ECS& ecs, const ChargeReleaseRequest& request);

        /**
         * @brief Process a weapon switch request.
         */
        void processSwitchWeapon(ECS::ECS& ecs, const SwitchWeaponRequest& request);

        /**
         * @brief Spawn a missile entity.
         * @return The spawned missile's entity ID
         */
        ECS::EntityID spawnMissile(ECS::ECS& ecs, uint8_t ownerId, float x, float y,
                                    uint8_t weaponType, uint8_t weaponLevel);

        /**
         * @brief Spawn a Wave Cannon beam.
         */
        ECS::EntityID spawnWaveCannon(ECS::ECS& ecs, uint8_t ownerId, float x, float y,
                                       uint8_t chargeLevel);

        /**
         * @brief Generate next missile ID.
         */
        uint16_t generateMissileId();

        /**
         * @brief Update cooldowns for all players.
         */
        void updateCooldowns(ECS::ECS& ecs, float deltaTime);

        /**
         * @brief Update charge timers for all charging players.
         */
        void updateChargeTimes(ECS::ECS& ecs, float deltaTime);
    };

}  // namespace infrastructure::ecs::systems

#endif /* !WEAPON_SYSTEM_HPP_ */
