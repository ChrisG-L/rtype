/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Transform - Composant de position et rotation
*/

#pragma once

namespace rtype::ecs::components {

/**
 * @brief Composant de transformation (position, rotation, échelle)
 *
 * Représente la position spatiale d'une entité dans le monde du jeu.
 */
struct Transform {
    float x = 0.0f;        // Position X
    float y = 0.0f;        // Position Y
    float rotation = 0.0f; // Rotation en degrés
    float scaleX = 1.0f;   // Échelle X
    float scaleY = 1.0f;   // Échelle Y

    Transform() = default;

    Transform(float x, float y, float rotation = 0.0f)
        : x(x), y(y), rotation(rotation) {}

    Transform(float x, float y, float rotation, float scaleX, float scaleY)
        : x(x), y(y), rotation(rotation), scaleX(scaleX), scaleY(scaleY) {}
};

} // namespace rtype::ecs::components
