/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** NetworkId - Composant d'identification réseau
*/

#pragma once

#include <cstdint>

namespace rtype::ecs::components {

/**
 * @brief Type d'entité réseau
 */
enum class NetworkEntityType : std::uint8_t {
    Player = 0,
    Enemy = 1,
    Missile = 2,
    Powerup = 3,
    Boss = 4
};

/**
 * @brief Composant d'identification réseau
 *
 * Lie une entité locale à son équivalent sur le serveur.
 * Essentiel pour la synchronisation client/serveur.
 */
struct NetworkId {
    std::uint32_t serverId = 0;              // ID assigné par le serveur
    NetworkEntityType type = NetworkEntityType::Player; // Type d'entité
    bool isLocalPlayer = false;              // Est-ce le joueur local ?
    bool needsInterpolation = true;          // Doit-on interpoler cette entité ?

    NetworkId() = default;

    NetworkId(std::uint32_t serverId, NetworkEntityType type = NetworkEntityType::Player)
        : serverId(serverId), type(type) {}

    NetworkId(std::uint32_t serverId, NetworkEntityType type, bool isLocalPlayer)
        : serverId(serverId), type(type), isLocalPlayer(isLocalPlayer) {}
};

} // namespace rtype::ecs::components
