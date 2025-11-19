/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** PlayerInput - Composant d'entrées joueur
*/

#pragma once

#include <cstdint>

namespace rtype::ecs::components {

/**
 * @brief Composant d'entrées joueur
 *
 * Stocke l'état actuel des entrées du joueur local.
 * Utilisé pour le client-side prediction.
 */
struct PlayerInput {
    // État actuel des touches (bitfield)
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool shoot = false;

    // Numéro de séquence pour la réconciliation serveur
    std::uint32_t sequenceNumber = 0;

    // Tick serveur correspondant
    std::uint32_t tick = 0;

    PlayerInput() = default;

    /**
     * @brief Encode les inputs en bitfield pour le réseau
     */
    [[nodiscard]] std::uint8_t toBitfield() const {
        std::uint8_t result = 0;
        if (up)    result |= (1 << 0);
        if (down)  result |= (1 << 1);
        if (left)  result |= (1 << 2);
        if (right) result |= (1 << 3);
        if (shoot) result |= (1 << 4);
        return result;
    }

    /**
     * @brief Décode un bitfield en inputs
     */
    void fromBitfield(std::uint8_t bitfield) {
        up    = (bitfield & (1 << 0)) != 0;
        down  = (bitfield & (1 << 1)) != 0;
        left  = (bitfield & (1 << 2)) != 0;
        right = (bitfield & (1 << 3)) != 0;
        shoot = (bitfield & (1 << 4)) != 0;
    }

    /**
     * @brief Vérifie si une entrée est active
     */
    [[nodiscard]] bool hasInput() const {
        return up || down || left || right || shoot;
    }
};

} // namespace rtype::ecs::components
