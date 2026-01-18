/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ScoreComp - ECS Component for player score tracking
*/

#ifndef SCORE_COMP_HPP_
#define SCORE_COMP_HPP_

#include <cstdint>

namespace infrastructure::ecs::components {

    /**
     * @brief Component storing player score and combo state.
     *
     * total: Accumulated score
     * kills: Total kills count
     * comboMultiplier: Current combo (1.0 - 3.0)
     * comboTimer: Time since last kill (for combo decay)
     */
    struct ScoreComp {
        uint32_t total = 0;           // Total score
        uint16_t kills = 0;           // Kill count
        float comboMultiplier = 1.0f; // Current combo (1.0 - 3.0x)
        float comboTimer = 0.0f;      // Time since last kill (seconds)
        float maxCombo = 1.0f;        // Best combo achieved this game
        uint8_t deaths = 0;           // Death count
    };

}  // namespace infrastructure::ecs::components

#endif /* !SCORE_COMP_HPP_ */
