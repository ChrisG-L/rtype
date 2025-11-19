/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Velocity - Composant de vélocité
*/

#pragma once

namespace rtype::ecs::components {

/**
 * @brief Composant de vélocité
 *
 * Représente la vitesse de déplacement d'une entité.
 */
struct Velocity {
    float vx = 0.0f; // Vélocité X (pixels/seconde)
    float vy = 0.0f; // Vélocité Y (pixels/seconde)

    Velocity() = default;

    Velocity(float vx, float vy)
        : vx(vx), vy(vy) {}
};

} // namespace rtype::ecs::components
