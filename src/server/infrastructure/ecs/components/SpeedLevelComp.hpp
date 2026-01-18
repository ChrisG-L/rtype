/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SpeedLevelComp - ECS Component for player speed upgrade level
*/

#ifndef SPEED_LEVEL_COMP_HPP_
#define SPEED_LEVEL_COMP_HPP_

#include <cstdint>

namespace infrastructure::ecs::components {

    /**
     * @brief Component storing player speed upgrade level.
     *
     * level: Speed upgrade level (0-3)
     *   Level 0: 1.0x speed (base 200 px/s)
     *   Level 1: 1.3x speed (260 px/s)
     *   Level 2: 1.6x speed (320 px/s)
     *   Level 3: 1.9x speed (380 px/s)
     */
    struct SpeedLevelComp {
        uint8_t level = 0;  // 0-3

        /**
         * @brief Upgrade speed level.
         * @return true if upgraded, false if already max (3)
         */
        bool upgrade() {
            if (level < 3) {
                level++;
                return true;
            }
            return false;
        }

        /**
         * @brief Check if at max level.
         */
        bool isMaxLevel() const {
            return level >= 3;
        }
    };

}  // namespace infrastructure::ecs::components

#endif /* !SPEED_LEVEL_COMP_HPP_ */
