/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** HitboxComp - ECS Component for collision detection
*/

#ifndef HITBOX_COMP_HPP_
#define HITBOX_COMP_HPP_

namespace infrastructure::ecs::components {

    /**
     * @brief Component storing entity hitbox dimensions.
     *
     * Used with PositionComp for AABB collision detection.
     * Offset is relative to entity position (usually 0,0).
     */
    struct HitboxComp {
        float width = 0.0f;
        float height = 0.0f;
        float offsetX = 0.0f;  // Hitbox offset from position X
        float offsetY = 0.0f;  // Hitbox offset from position Y
    };

}  // namespace infrastructure::ecs::components

#endif /* !HITBOX_COMP_HPP_ */
