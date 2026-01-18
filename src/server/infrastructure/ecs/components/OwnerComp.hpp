/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** OwnerComp - ECS Component for entity ownership
*/

#ifndef OWNER_COMP_HPP_
#define OWNER_COMP_HPP_

#include <cstdint>

namespace infrastructure::ecs::components {

    /**
     * @brief Component storing entity ownership information.
     *
     * Used for: Missiles (who fired), Force Pods (attached to), etc.
     * ownerId is an EntityID of the owning entity.
     */
    struct OwnerComp {
        uint32_t ownerId = 0;        // EntityID of owner (0 = no owner)
        bool isPlayerOwned = true;   // true = player, false = enemy/environment
    };

}  // namespace infrastructure::ecs::components

#endif /* !OWNER_COMP_HPP_ */
