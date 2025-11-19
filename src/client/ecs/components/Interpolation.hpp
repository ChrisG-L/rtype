/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Interpolation - Composant pour l'interpolation réseau
*/

#pragma once

#include <deque>
#include <cstdint>

namespace rtype::ecs::components {

/**
 * @brief État d'une entité à un instant donné
 */
struct NetworkState {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    std::uint32_t tick = 0;    // Tick serveur
    float timestamp = 0.0f;     // Temps local de réception
};

/**
 * @brief Composant d'interpolation pour les entités distantes
 *
 * Stocke les états passés pour permettre une interpolation fluide
 * des mouvements des autres joueurs et ennemis.
 */
struct Interpolation {
    std::deque<NetworkState> stateBuffer;  // Buffer d'états (du plus ancien au plus récent)
    float interpolationDelay = 0.1f;       // Délai d'interpolation (100ms = ~6 frames à 60Hz)
    std::size_t maxBufferSize = 20;        // Taille max du buffer

    Interpolation() = default;

    Interpolation(float delay)
        : interpolationDelay(delay) {}

    /**
     * @brief Ajoute un nouvel état au buffer
     */
    void addState(const NetworkState& state) {
        stateBuffer.push_back(state);

        // Limite la taille du buffer
        while (stateBuffer.size() > maxBufferSize) {
            stateBuffer.pop_front();
        }
    }

    /**
     * @brief Vide le buffer
     */
    void clear() {
        stateBuffer.clear();
    }
};

} // namespace rtype::ecs::components
