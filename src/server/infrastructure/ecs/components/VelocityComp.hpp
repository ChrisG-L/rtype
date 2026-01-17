/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** VelocityComp - ECS Component for entity velocity
*/

#ifndef VELOCITY_COMP_HPP_
#define VELOCITY_COMP_HPP_

namespace infrastructure::ecs::components {

    /**
     * @brief Component storing entity velocity.
     *
     * Velocity in pixels per second.
     * Positive X = moving right, Positive Y = moving down.
     */
    struct VelocityComp {
        float x = 0.0f;
        float y = 0.0f;
    };

}  // namespace infrastructure::ecs::components

#endif /* !VELOCITY_COMP_HPP_ */
