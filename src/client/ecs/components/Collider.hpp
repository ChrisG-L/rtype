/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Collider - Composant de collision
*/

#pragma once

#include <cstdint>

namespace rtype::ecs::components {

/**
 * @brief Types de collision
 */
enum class CollisionLayer : std::uint8_t {
    None     = 0,
    Player   = 1 << 0, // 1
    Enemy    = 1 << 1, // 2
    Missile  = 1 << 2, // 4
    Powerup  = 1 << 3, // 8
    Wall     = 1 << 4  // 16
};

/**
 * @brief Composant de collision (boîte englobante)
 *
 * Définit la zone de collision d'une entité.
 */
struct Collider {
    float width = 0.0f;                // Largeur de la boîte
    float height = 0.0f;               // Hauteur de la boîte
    float offsetX = 0.0f;              // Décalage X par rapport à Transform
    float offsetY = 0.0f;              // Décalage Y par rapport à Transform
    CollisionLayer layer = CollisionLayer::None;  // Couche de collision
    std::uint8_t mask = 0;             // Masque de collision (avec quelles couches collisionner)

    Collider() = default;

    Collider(float width, float height, CollisionLayer layer = CollisionLayer::None)
        : width(width), height(height), layer(layer) {}

    Collider(float width, float height, CollisionLayer layer, std::uint8_t mask)
        : width(width), height(height), layer(layer), mask(mask) {}
};

/**
 * @brief Opérateur pour combiner les couches de collision
 */
inline std::uint8_t operator|(CollisionLayer a, CollisionLayer b) {
    return static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b);
}

} // namespace rtype::ecs::components
