/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** CollisionRule - Domain service for collision detection
*/

#ifndef COLLISIONRULE_HPP_
#define COLLISIONRULE_HPP_

#include <cstdint>

namespace domain::services {

    /**
     * @brief Domain representation of a hitbox.
     * Independent from infrastructure AABB to maintain hexagonal architecture.
     */
    struct HitboxData {
        float x;
        float y;
        float width;
        float height;

        constexpr HitboxData(float px, float py, float w, float h)
            : x(px), y(py), width(w), height(h) {}
    };

    /**
     * @brief Domain service for collision detection.
     *
     * Provides AABB (Axis-Aligned Bounding Box) collision detection
     * and boundary checking. This is a Domain abstraction over the
     * infrastructure collision::AABB class.
     *
     * All methods are const and stateless.
     */
    class CollisionRule {
    public:
        CollisionRule() = default;
        ~CollisionRule() = default;

        /**
         * @brief Check if two hitboxes are colliding (AABB intersection).
         * @param a First hitbox
         * @param b Second hitbox
         * @return true if hitboxes overlap
         */
        bool checkAABB(const HitboxData& a, const HitboxData& b) const;

        /**
         * @brief Check if a hitbox is fully outside screen bounds.
         * @param hitbox The hitbox to check
         * @return true if entirely outside screen (can be despawned)
         */
        bool isOutOfBounds(const HitboxData& hitbox) const;

        /**
         * @brief Check if a hitbox is partially outside screen bounds.
         * @param hitbox The hitbox to check
         * @return true if any part is outside screen
         */
        bool isPartiallyOutOfBounds(const HitboxData& hitbox) const;

        /**
         * @brief Check if a point is inside a hitbox.
         * @param hitbox The hitbox
         * @param px Point X coordinate
         * @param py Point Y coordinate
         * @return true if point is inside hitbox
         */
        bool containsPoint(const HitboxData& hitbox, float px, float py) const;

        /**
         * @brief Clamp a position to keep hitbox within screen bounds.
         * @param x Current X position (modified in place)
         * @param y Current Y position (modified in place)
         * @param width Hitbox width
         * @param height Hitbox height
         */
        void clampToScreen(float& x, float& y, float width, float height) const;

        /**
         * @brief Get screen width constant.
         * @return Screen width in pixels
         */
        float getScreenWidth() const;

        /**
         * @brief Get screen height constant.
         * @return Screen height in pixels
         */
        float getScreenHeight() const;
    };

}  // namespace domain::services

#endif /* !COLLISIONRULE_HPP_ */
