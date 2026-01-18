/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** LifetimeComp - ECS Component for timed entities
*/

#ifndef LIFETIME_COMP_HPP_
#define LIFETIME_COMP_HPP_

namespace infrastructure::ecs::components {

    /**
     * @brief Component for entities with limited lifetime.
     *
     * Used for: Power-ups, temporary effects, etc.
     * Entity should be destroyed when remaining <= 0.
     */
    struct LifetimeComp {
        float remaining = 0.0f;  // Seconds until expiration
        float total = 0.0f;      // Original lifetime (for progress bars)
    };

}  // namespace infrastructure::ecs::components

#endif /* !LIFETIME_COMP_HPP_ */
