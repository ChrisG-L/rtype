/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** CollisionRule - Implementation
*/

#include "domain/services/CollisionRule.hpp"
#include "domain/Constants.hpp"
#include <algorithm>

namespace domain::services {

    using namespace domain::constants;

    bool CollisionRule::checkAABB(const HitboxData& a, const HitboxData& b) const {
        // Standard AABB intersection test
        return a.x < b.x + b.width &&
               a.x + a.width > b.x &&
               a.y < b.y + b.height &&
               a.y + a.height > b.y;
    }

    bool CollisionRule::isOutOfBounds(const HitboxData& hitbox) const {
        // Fully outside screen (left, right, top, or bottom)
        return hitbox.x + hitbox.width < 0.0f ||
               hitbox.x > world::SCREEN_WIDTH ||
               hitbox.y + hitbox.height < 0.0f ||
               hitbox.y > world::SCREEN_HEIGHT;
    }

    bool CollisionRule::isPartiallyOutOfBounds(const HitboxData& hitbox) const {
        // Any part outside screen
        return hitbox.x < 0.0f ||
               hitbox.x + hitbox.width > world::SCREEN_WIDTH ||
               hitbox.y < 0.0f ||
               hitbox.y + hitbox.height > world::SCREEN_HEIGHT;
    }

    bool CollisionRule::containsPoint(const HitboxData& hitbox, float px, float py) const {
        return px >= hitbox.x && px <= hitbox.x + hitbox.width &&
               py >= hitbox.y && py <= hitbox.y + hitbox.height;
    }

    void CollisionRule::clampToScreen(float& x, float& y, float width, float height) const {
        x = std::clamp(x, 0.0f, world::SCREEN_WIDTH - width);
        y = std::clamp(y, 0.0f, world::SCREEN_HEIGHT - height);
    }

    float CollisionRule::getScreenWidth() const {
        return world::SCREEN_WIDTH;
    }

    float CollisionRule::getScreenHeight() const {
        return world::SCREEN_HEIGHT;
    }

}  // namespace domain::services
