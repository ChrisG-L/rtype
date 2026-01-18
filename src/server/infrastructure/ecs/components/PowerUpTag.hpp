/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** PowerUpTag - ECS Component for power-up identification
*/

#ifndef POWER_UP_TAG_HPP_
#define POWER_UP_TAG_HPP_

#include <cstdint>

namespace infrastructure::ecs::components {

    /**
     * @brief Tag component for power-up entities.
     *
     * type: Power-up type (PowerUpType enum)
     *   0 = Health (restore 25 HP)
     *   1 = SpeedUp (increase speed level)
     *   2 = WeaponCrystal (increase weapon level)
     *   3 = ForcePod (acquire/upgrade Force Pod)
     *   4 = BitDevice (acquire Bit Devices)
     */
    struct PowerUpTag {
        uint8_t type = 0;    // PowerUpType enum
    };

}  // namespace infrastructure::ecs::components

#endif /* !POWER_UP_TAG_HPP_ */
