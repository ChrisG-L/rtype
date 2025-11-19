/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Starfield - Implémentation de l'effet champ d'étoiles
*/

#include "Starfield.hpp"
#include <chrono>

namespace rtype::graphics {

Starfield::Starfield(float width, float height, std::size_t starCount)
    : m_width(width)
    , m_height(height)
    , m_starCount(starCount)
{
    // Initialise le générateur aléatoire avec une seed basée sur le temps
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    m_rng.seed(static_cast<std::mt19937::result_type>(seed));

    // Configure la shape par défaut
    m_starShape.setPointCount(4);

    // Génère les étoiles
    initStars();
}

void Starfield::initStars() {
    m_stars.reserve(m_starCount);

    std::uniform_real_distribution<float> xDist(0.0f, m_width);
    std::uniform_real_distribution<float> yDist(0.0f, m_height);

    for (std::size_t i = 0; i < m_starCount; ++i) {
        Star star;
        star.x = xDist(m_rng);
        star.y = yDist(m_rng);

        // Génère les propriétés basées sur la "profondeur"
        respawnStar(star, true);

        m_stars.push_back(star);
    }
}

void Starfield::respawnStar(Star& star, bool randomX) {
    std::uniform_real_distribution<float> depthDist(0.0f, 1.0f);
    float depth = depthDist(m_rng);

    // Plus l'étoile est "proche" (depth élevé), plus elle est grande et rapide
    star.speed = 50.0f + depth * 200.0f;  // 50-250 pixels/seconde
    star.size = 1.0f + depth * 2.0f;       // 1-3 pixels
    star.brightness = static_cast<std::uint8_t>(100 + depth * 155); // 100-255

    if (randomX) {
        std::uniform_real_distribution<float> xDist(0.0f, m_width);
        star.x = xDist(m_rng);
    } else {
        // Réapparaît à droite
        star.x = m_width + star.size;
    }

    // Position Y aléatoire
    std::uniform_real_distribution<float> yDist(0.0f, m_height);
    star.y = yDist(m_rng);
}

void Starfield::update(float dt) {
    float effectiveDt = dt * m_speedMultiplier;

    for (auto& star : m_stars) {
        // Déplace l'étoile vers la gauche
        star.x -= star.speed * effectiveDt;

        // Réapparaît à droite si sortie de l'écran
        if (star.x < -star.size) {
            respawnStar(star, false);
        }
    }
}

void Starfield::draw(sf::RenderTarget& target) {
    for (const auto& star : m_stars) {
        m_starShape.setRadius(star.size);
        m_starShape.setPosition({star.x - star.size, star.y - star.size});
        m_starShape.setFillColor(sf::Color(
            star.brightness,
            star.brightness,
            star.brightness
        ));

        target.draw(m_starShape);
    }
}

void Starfield::reset() {
    m_stars.clear();
    initStars();
}

} // namespace rtype::graphics
