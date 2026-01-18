/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MissileTag - ECS Component for missile identification
*/

#ifndef MISSILE_TAG_HPP_
#define MISSILE_TAG_HPP_

#include <cstdint>

namespace infrastructure::ecs::components {

    /**
     * @brief Tag component for missile entities.
     *
     * weaponType: Type of weapon that fired this missile (WeaponType enum)
     * baseDamage: Base damage before level multipliers
     * isHoming: If true, missile tracks targets
     * targetId: EntityID of target for homing missiles (0 = no target)
     */
    struct MissileTag {
        uint8_t weaponType = 0;      // WeaponType enum
        uint8_t baseDamage = 20;     // Base damage value
        bool isHoming = false;       // Homing behavior
        uint32_t targetId = 0;       // Target EntityID for homing (0 = none)
    };

}  // namespace infrastructure::ecs::components

#endif /* !MISSILE_TAG_HPP_ */
