/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** InterpolationSystem - Système d'interpolation réseau
*/

#pragma once

#include "../System.hpp"
#include "../Registry.hpp"
#include "../components/Transform.hpp"
#include "../components/Interpolation.hpp"
#include "../components/NetworkId.hpp"
#include <cmath>

namespace rtype::ecs::systems {

/**
 * @brief Système d'interpolation pour les entités distantes
 *
 * Interpole les positions des entités non-locales entre les états
 * reçus du serveur pour un mouvement fluide.
 * Utilise une technique de "render in the past" (100ms de délai).
 */
class InterpolationSystem : public ISystem {
public:
    InterpolationSystem() = default;

    void update(Registry& registry, float dt) override {
        m_currentTime += dt;

        registry.view<components::Transform, components::Interpolation, components::NetworkId>(
            [this](Entity /*entity*/, components::Transform& transform,
                   components::Interpolation& interp, components::NetworkId& netId) {
                // Ne pas interpoler le joueur local
                if (netId.isLocalPlayer || !netId.needsInterpolation) return;

                if (interp.stateBuffer.size() < 2) return;

                // Temps cible = maintenant - délai d'interpolation
                float targetTime = m_currentTime - interp.interpolationDelay;

                // Trouve les deux états encadrant le temps cible
                const components::NetworkState* before = nullptr;
                const components::NetworkState* after = nullptr;

                for (std::size_t i = 0; i < interp.stateBuffer.size() - 1; ++i) {
                    if (interp.stateBuffer[i].timestamp <= targetTime &&
                        interp.stateBuffer[i + 1].timestamp >= targetTime) {
                        before = &interp.stateBuffer[i];
                        after = &interp.stateBuffer[i + 1];
                        break;
                    }
                }

                // Si pas d'états encadrants, utilise extrapolation
                if (!before || !after) {
                    // Extrapolation basée sur le dernier état
                    if (!interp.stateBuffer.empty()) {
                        const auto& last = interp.stateBuffer.back();
                        float elapsed = m_currentTime - last.timestamp;

                        // Extrapolation simple (peut causer des artefacts)
                        transform.x = last.x + last.vx * elapsed;
                        transform.y = last.y + last.vy * elapsed;
                    }
                    return;
                }

                // Interpolation linéaire entre les deux états
                float totalTime = after->timestamp - before->timestamp;
                // Epsilon pour éviter la division par des valeurs trop petites
                constexpr float EPSILON = 0.001f;
                float t = (totalTime > EPSILON) ? (targetTime - before->timestamp) / totalTime : 0.0f;

                // Clamp t entre 0 et 1
                t = std::max(0.0f, std::min(t, 1.0f));

                // Interpolation linéaire (lerp)
                transform.x = before->x + (after->x - before->x) * t;
                transform.y = before->y + (after->y - before->y) * t;
            });
    }

    [[nodiscard]] int priority() const override {
        return 15; // Après InputSystem, avant AnimationSystem
    }

    /**
     * @brief Réinitialise le temps courant
     */
    void resetTime() {
        m_currentTime = 0.0f;
    }

    /**
     * @brief Synchronise le temps avec le serveur
     */
    void setCurrentTime(float time) {
        m_currentTime = time;
    }

private:
    float m_currentTime = 0.0f;
};

} // namespace rtype::ecs::systems
