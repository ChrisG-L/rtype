/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Missile - Composant de projectile
*/

#pragma once

#include <cstdint>
#include "../Entity.hpp"

namespace rtype::ecs::components {

/**
 * @brief Type de missile
 */
enum class MissileType : std::uint8_t {
    Basic = 0,      // Tir basique
    Charged = 1,    // Tir chargé
    Enemy = 2       // Tir ennemi
};

/**
 * @brief Composant de missile/projectile
 *
 * Représente un projectile dans le jeu.
 */
struct Missile {
    MissileType type = MissileType::Basic; // Type de missile
    std::int32_t damage = 10;              // Dégâts infligés
    Entity owner = 0;                      // Entité qui a tiré le missile
    float lifetime = 5.0f;                 // Durée de vie restante (secondes)

    Missile() = default;

    Missile(MissileType type, std::int32_t damage, Entity owner = 0)
        : type(type), damage(damage), owner(owner) {}
};

} // namespace rtype::ecs::components
