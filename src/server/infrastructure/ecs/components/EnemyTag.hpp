/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** EnemyTag - ECS Component for enemy identification
*/

#ifndef ENEMY_TAG_HPP_
#define ENEMY_TAG_HPP_

#include <cstdint>

namespace infrastructure::ecs::components {

    /**
     * @brief Tag component for enemy entities.
     *
     * type: Enemy type (EnemyType enum: Basic=0, Tracker=1, Zigzag=2, Fast=3, Bomber=4, POWArmor=5)
     * points: Score points awarded when this enemy is killed
     */
    struct EnemyTag {
        uint8_t type = 0;        // EnemyType enum
        uint16_t points = 100;   // Score value when killed
    };

}  // namespace infrastructure::ecs::components

#endif /* !ENEMY_TAG_HPP_ */
