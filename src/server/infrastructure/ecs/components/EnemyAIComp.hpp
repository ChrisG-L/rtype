/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** EnemyAIComp - ECS Component for enemy AI state
*/

#ifndef ENEMY_AI_COMP_HPP_
#define ENEMY_AI_COMP_HPP_

#include <cstdint>

namespace infrastructure::ecs::components {

    /**
     * @brief Component storing enemy AI state for behavior systems.
     *
     * Controls shooting behavior, movement patterns, and timing.
     * Works in conjunction with EnemyTag for type-specific behavior.
     */
    struct EnemyAIComp {
        // Shooting
        float shootCooldown = 0.0f;      // Time until next shot allowed
        float shootInterval = 2.5f;      // Time between shots (type-dependent)

        // Movement pattern state
        uint8_t movementPattern = 0;     // Pattern type (0=linear, 1=sine, 2=zigzag, 3=track)
        float patternTimer = 0.0f;       // General pattern timing
        float baseY = 0.0f;              // Y position at spawn (for sine/zigzag)
        float aliveTime = 0.0f;          // Total time alive (for phase calculations)
        float phaseOffset = 0.0f;        // Random offset for wave movement

        // Tracker-specific
        float targetY = 0.0f;            // Target Y position for tracking enemies

        // Zigzag-specific
        float zigzagTimer = 0.0f;        // Timer for direction changes
        bool zigzagUp = true;            // Current zigzag direction
    };

}  // namespace infrastructure::ecs::components

#endif /* !ENEMY_AI_COMP_HPP_ */
