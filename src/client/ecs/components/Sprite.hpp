/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Sprite - Composant de rendu visuel
*/

#pragma once

#include <string>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Color.hpp>

namespace rtype::ecs::components {

/**
 * @brief Composant de sprite pour le rendu
 *
 * Contient les informations nécessaires pour afficher une entité.
 */
struct Sprite {
    std::string textureId;           // ID de la texture dans l'AssetManager
    sf::IntRect textureRect;         // Rectangle source dans la texture
    sf::Color color = sf::Color::White; // Couleur de teinte
    int zOrder = 0;                  // Ordre de rendu (plus haut = devant)
    bool visible = true;             // Visibilité

    Sprite() = default;

    Sprite(const std::string& textureId, int zOrder = 0)
        : textureId(textureId), zOrder(zOrder) {}

    Sprite(const std::string& textureId, const sf::IntRect& rect, int zOrder = 0)
        : textureId(textureId), textureRect(rect), zOrder(zOrder) {}
};

} // namespace rtype::ecs::components
