/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Entity - Définition du type Entity pour l'ECS
*/

#pragma once

#include <cstdint>
#include <limits>

namespace rtype::ecs {

/**
 * @brief L'Entity est simplement un identifiant (ID)
 *
 * Dans l'architecture ECS, les entités sont des identifiants légers
 * qui servent de clés pour accéder aux composants.
 */
using Entity = std::uint32_t;

/**
 * @brief Constante pour une entité invalide
 */
constexpr Entity NULL_ENTITY = std::numeric_limits<Entity>::max();

/**
 * @brief Nombre maximum d'entités
 */
constexpr std::size_t MAX_ENTITIES = 10000;

/**
 * @brief Identifiant du type de composant
 */
using ComponentType = std::uint8_t;

/**
 * @brief Nombre maximum de types de composants
 */
constexpr ComponentType MAX_COMPONENTS = 64;

} // namespace rtype::ecs
