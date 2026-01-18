/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** PlayerInputSystem - ECS System for processing player inputs
*/

#ifndef PLAYER_INPUT_SYSTEM_HPP_
#define PLAYER_INPUT_SYSTEM_HPP_

#include "core/System.hpp"
#include "bridge/DomainBridge.hpp"
#include <cstdint>
#include <vector>
#include <queue>
#include <optional>

namespace infrastructure::ecs::systems {

    /**
     * @brief Input bitfield constants (matches Protocol.hpp InputKeys).
     */
    namespace InputKeys {
        constexpr uint16_t UP    = 0x0001;
        constexpr uint16_t DOWN  = 0x0002;
        constexpr uint16_t LEFT  = 0x0004;
        constexpr uint16_t RIGHT = 0x0008;
        constexpr uint16_t SHOOT = 0x0010;
    }

    /**
     * @brief Represents a player input event.
     */
    struct PlayerInputEvent {
        uint8_t playerId;       // Player ID (1-4)
        uint16_t keys;          // Input bitfield
        uint16_t sequenceNum;   // For client-side prediction
    };

    /**
     * @brief System that processes player inputs and updates velocities.
     *
     * Query: <PlayerTag, PositionComp, VelocityComp, SpeedLevelComp>
     * Priority: 0 (runs first, before MovementSystem)
     *
     * Converts input keys to velocity vectors.
     * Clamps positions to screen bounds after movement.
     * Uses DomainBridge for speed calculations.
     */
    class PlayerInputSystem : public ECS::ISystem {
    public:
        /**
         * @brief Construct PlayerInputSystem with a DomainBridge.
         * @param bridge Reference to DomainBridge for speed calculations
         */
        explicit PlayerInputSystem(bridge::DomainBridge& bridge);
        ~PlayerInputSystem() override = default;

        /**
         * @brief Process pending inputs and update player velocities.
         *
         * @param ecs The ECS instance
         * @param thisID This system's ID (unused)
         * @param msecs Delta time in milliseconds
         */
        void Update(ECS::ECS& ecs, ECS::SystemID thisID, uint32_t msecs) override;

        /**
         * @brief Queue an input event for processing.
         * @param event The input event to process
         */
        void queueInput(const PlayerInputEvent& event);

        /**
         * @brief Clear all pending inputs.
         */
        void clearInputs();

        /**
         * @brief Set whether to auto-clamp positions to screen bounds.
         * @param enabled True to clamp, false to allow off-screen
         */
        void setClampEnabled(bool enabled);

    private:
        bridge::DomainBridge& _bridge;
        std::queue<PlayerInputEvent> _inputQueue;
        bool _clampEnabled = true;

        /**
         * @brief Find player entity by player ID.
         * @return EntityID if found, std::nullopt otherwise
         */
        std::optional<ECS::EntityID> findPlayerByID(ECS::ECS& ecs, uint8_t playerId);

        /**
         * @brief Process a single input event.
         */
        void processInput(ECS::ECS& ecs, const PlayerInputEvent& event);
    };

}  // namespace infrastructure::ecs::systems

#endif /* !PLAYER_INPUT_SYSTEM_HPP_ */
