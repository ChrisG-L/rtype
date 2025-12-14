/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** AABB - Axis-Aligned Bounding Box for collision detection
*/

#ifndef AABB_HPP_
#define AABB_HPP_

#include <cstdint>

namespace collision {

struct AABB {
    float x;
    float y;
    float width;
    float height;

    constexpr AABB(float px, float py, float w, float h)
        : x(px), y(py), width(w), height(h) {}

    constexpr bool intersects(const AABB& other) const {
        return x < other.x + other.width &&
               x + width > other.x &&
               y < other.y + other.height &&
               y + height > other.y;
    }

    constexpr bool contains(float px, float py) const {
        return px >= x && px <= x + width &&
               py >= y && py <= y + height;
    }

    constexpr float centerX() const { return x + width / 2.0f; }
    constexpr float centerY() const { return y + height / 2.0f; }

    constexpr void setPosition(float px, float py) {
        x = px;
        y = py;
    }
};

namespace Hitboxes {
    static constexpr float SHIP_WIDTH = 64.0f;
    static constexpr float SHIP_HEIGHT = 30.0f;
    static constexpr float MISSILE_WIDTH = 16.0f;
    static constexpr float MISSILE_HEIGHT = 8.0f;
    static constexpr float ENEMY_WIDTH = 40.0f;
    static constexpr float ENEMY_HEIGHT = 40.0f;
}

}

#endif /* !AABB_HPP_ */
