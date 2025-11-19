/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Starfield - Effet de champ d'étoiles animé
*/

#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <vector>
#include <random>

namespace rtype::graphics {

/**
 * @brief Une étoile dans le champ d'étoiles
 */
struct Star {
    float x;
    float y;
    float speed;      // Vitesse de défilement
    float size;       // Taille de l'étoile
    std::uint8_t brightness; // Luminosité (0-255)
};

/**
 * @brief Effet visuel de champ d'étoiles avec parallaxe
 *
 * Crée un effet de profondeur avec plusieurs couches d'étoiles
 * qui défilent à différentes vitesses.
 */
class Starfield {
public:
    /**
     * @brief Constructeur
     * @param width Largeur de la zone
     * @param height Hauteur de la zone
     * @param starCount Nombre d'étoiles
     */
    Starfield(float width, float height, std::size_t starCount = 200);

    /**
     * @brief Met à jour les positions des étoiles
     * @param dt Delta time en secondes
     */
    void update(float dt);

    /**
     * @brief Dessine les étoiles
     * @param target Cible de rendu
     */
    void draw(sf::RenderTarget& target);

    /**
     * @brief Définit la vitesse de défilement
     * @param speed Multiplicateur de vitesse (1.0 = normal)
     */
    void setSpeed(float speed) { m_speedMultiplier = speed; }

    /**
     * @brief Réinitialise les étoiles
     */
    void reset();

private:
    void initStars();
    void respawnStar(Star& star, bool randomX = false);

private:
    float m_width;
    float m_height;
    std::size_t m_starCount;
    float m_speedMultiplier = 1.0f;

    std::vector<Star> m_stars;

    // Générateur aléatoire
    std::mt19937 m_rng;

    // Shapes pour le rendu
    sf::CircleShape m_starShape;
};

} // namespace rtype::graphics
