/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** HealthComp - ECS Component for entity health
*/

#ifndef HEALTH_COMP_HPP_
#define HEALTH_COMP_HPP_

#include <cstdint>

namespace infrastructure::ecs::components {

    /**
     * @brief Component storing entity health.
     *
     * current: Current health points
     * max: Maximum health points
     * invulnerable: If true, entity takes no damage (GodMode)
     */
    struct HealthComp {
        uint16_t current = 100;
        uint16_t max = 100;
        bool invulnerable = false;
    };

}  // namespace infrastructure::ecs::components

#endif /* !HEALTH_COMP_HPP_ */
