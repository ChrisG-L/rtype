/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Animation - Composant d'animation de sprites
*/

#pragma once

#include <vector>
#include <SFML/Graphics/Rect.hpp>

namespace rtype::ecs::components {

/**
 * @brief Composant d'animation
 *
 * Gère l'animation par frames d'un sprite.
 */
struct Animation {
    std::vector<sf::IntRect> frames;  // Liste des frames
    float frameTime = 0.1f;           // Durée de chaque frame (secondes)
    float currentTime = 0.0f;         // Temps écoulé depuis le début de la frame
    std::size_t currentFrame = 0;     // Index de la frame actuelle
    bool loop = true;                 // L'animation boucle-t-elle ?
    bool playing = true;              // L'animation est-elle en cours ?

    Animation() = default;

    Animation(const std::vector<sf::IntRect>& frames, float frameTime = 0.1f, bool loop = true)
        : frames(frames), frameTime(frameTime), loop(loop) {}

    /**
     * @brief Récupère la frame actuelle
     */
    [[nodiscard]] const sf::IntRect& getCurrentRect() const {
        return frames[currentFrame];
    }

    /**
     * @brief Réinitialise l'animation
     */
    void reset() {
        currentFrame = 0;
        currentTime = 0.0f;
        playing = true;
    }
};

} // namespace rtype::ecs::components
