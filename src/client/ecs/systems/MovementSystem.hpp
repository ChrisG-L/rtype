/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** MovementSystem - Système de déplacement
*/

#pragma once

#include "../System.hpp"
#include "../Registry.hpp"
#include "../components/Transform.hpp"
#include "../components/Velocity.hpp"
#include "../components/PlayerInput.hpp"
#include "../components/NetworkId.hpp"

namespace rtype::ecs::systems {

/**
 * @brief Système de déplacement des entités
 *
 * Applique la vélocité aux transforms.
 * Pour le joueur local, applique également les inputs (client-side prediction).
 */
class MovementSystem : public ISystem {
public:
    MovementSystem(float playerSpeed = 300.0f)
        : m_playerSpeed(playerSpeed) {}

    void update(Registry& registry, float dt) override {
        // Déplacement basé sur la vélocité
        registry.view<components::Transform, components::Velocity>(
            [dt](Entity /*entity*/, components::Transform& transform, components::Velocity& velocity) {
                transform.x += velocity.vx * dt;
                transform.y += velocity.vy * dt;
            });

        // Client-side prediction pour le joueur local
        registry.view<components::Transform, components::PlayerInput, components::NetworkId>(
            [this, dt](Entity /*entity*/, components::Transform& transform,
                       components::PlayerInput& input, components::NetworkId& netId) {
                if (!netId.isLocalPlayer) return;

                // Applique les inputs localement (prédiction)
                float dx = 0.0f;
                float dy = 0.0f;

                if (input.up)    dy -= m_playerSpeed * dt;
                if (input.down)  dy += m_playerSpeed * dt;
                if (input.left)  dx -= m_playerSpeed * dt;
                if (input.right) dx += m_playerSpeed * dt;

                transform.x += dx;
                transform.y += dy;

                // Limite aux bords de l'écran (à ajuster selon la résolution)
                transform.x = std::max(0.0f, std::min(transform.x, m_screenWidth - m_playerWidth));
                transform.y = std::max(0.0f, std::min(transform.y, m_screenHeight - m_playerHeight));
            });
    }

    [[nodiscard]] int priority() const override {
        return 10; // Après InputSystem
    }

    /**
     * @brief Configure les limites de l'écran
     */
    void setScreenBounds(float width, float height) {
        m_screenWidth = width;
        m_screenHeight = height;
    }

    /**
     * @brief Configure la taille du joueur (pour les limites)
     */
    void setPlayerSize(float width, float height) {
        m_playerWidth = width;
        m_playerHeight = height;
    }

private:
    float m_playerSpeed = 300.0f;
    float m_screenWidth = 1920.0f;
    float m_screenHeight = 1080.0f;
    float m_playerWidth = 64.0f;
    float m_playerHeight = 64.0f;
};

} // namespace rtype::ecs::systems
