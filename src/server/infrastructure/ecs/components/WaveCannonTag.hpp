/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** WaveCannonTag - ECS Component for Wave Cannon projectiles
*/

#ifndef WAVE_CANNON_TAG_HPP_
#define WAVE_CANNON_TAG_HPP_

#include <cstdint>

namespace infrastructure::ecs::components {

    /**
     * @brief Tag component for Wave Cannon (charged beam) projectiles.
     *
     * chargeLevel: Charge level (1-3), determines damage and width
     *   Level 1: 50 damage, 20.0 width
     *   Level 2: 100 damage, 35.0 width
     *   Level 3: 250 damage, 55.0 width
     * width: Beam width in pixels (computed from chargeLevel)
     */
    struct WaveCannonTag {
        uint8_t chargeLevel = 1;     // Charge level (1-3)
        float width = 20.0f;         // Beam width in pixels
    };

}  // namespace infrastructure::ecs::components

#endif /* !WAVE_CANNON_TAG_HPP_ */
