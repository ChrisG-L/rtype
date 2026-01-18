/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** PlayerTag - ECS Component for player identification
*/

#ifndef PLAYER_TAG_HPP_
#define PLAYER_TAG_HPP_

#include <cstdint>

namespace infrastructure::ecs::components {

    /**
     * @brief Tag component for player entities.
     *
     * playerId: Network/game player ID (1-4 for local, 0 = invalid)
     * shipSkin: Visual skin selection (1-6)
     * isAlive: Player is currently alive and active
     */
    struct PlayerTag {
        uint8_t playerId = 0;    // Player ID (1-4)
        uint8_t shipSkin = 1;    // Ship skin (1-6)
        bool isAlive = true;     // Currently alive
    };

}  // namespace infrastructure::ecs::components

#endif /* !PLAYER_TAG_HPP_ */
