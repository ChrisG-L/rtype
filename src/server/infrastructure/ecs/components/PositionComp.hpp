/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** PositionComp - ECS Component for entity position
*/

#ifndef POSITION_COMP_HPP_
#define POSITION_COMP_HPP_

namespace infrastructure::ecs::components {

    /**
     * @brief Component storing entity position.
     *
     * Uses float for calculation precision.
     * Conversion to uint16_t happens only in NetworkSyncSystem.
     */
    struct PositionComp {
        float x = 0.0f;
        float y = 0.0f;
    };

}  // namespace infrastructure::ecs::components

#endif /* !POSITION_COMP_HPP_ */
