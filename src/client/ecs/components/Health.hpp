/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Health - Composant de santé
*/

#pragma once

#include <cstdint>

namespace rtype::ecs::components {

/**
 * @brief Composant de santé
 *
 * Représente les points de vie d'une entité.
 */
struct Health {
    std::int32_t current = 100;      // Points de vie actuels
    std::int32_t max = 100;          // Points de vie maximum
    bool invincible = false;         // Invincibilité temporaire
    float invincibleTimer = 0.0f;    // Temps restant d'invincibilité

    Health() = default;

    Health(std::int32_t maxHealth)
        : current(maxHealth), max(maxHealth) {}

    Health(std::int32_t current, std::int32_t max)
        : current(current), max(max) {}

    /**
     * @brief Vérifie si l'entité est morte
     */
    [[nodiscard]] bool isDead() const {
        return current <= 0;
    }

    /**
     * @brief Inflige des dégâts
     */
    void damage(std::int32_t amount) {
        if (!invincible) {
            current -= amount;
            if (current < 0) current = 0;
        }
    }

    /**
     * @brief Soigne l'entité
     */
    void heal(std::int32_t amount) {
        current += amount;
        if (current > max) current = max;
    }
};

} // namespace rtype::ecs::components
