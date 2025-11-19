/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** MissileSystem - Système de gestion des missiles
*/

#pragma once

#include "../System.hpp"
#include "../Registry.hpp"
#include "../components/Transform.hpp"
#include "../components/Missile.hpp"

namespace rtype::ecs::systems {

/**
 * @brief Système de gestion des missiles
 *
 * Met à jour la durée de vie des missiles et les détruit quand expirés.
 */
class MissileSystem : public ISystem {
public:
    MissileSystem() = default;

    void update(Registry& registry, float dt) override {
        registry.view<components::Missile>(
            [&registry, dt](Entity entity, components::Missile& missile) {
                missile.lifetime -= dt;

                // Planifie la destruction si expiré
                if (missile.lifetime <= 0.0f) {
                    registry.scheduleDestroy(entity);
                }
            });

        // Détruit aussi les missiles hors écran
        registry.view<components::Transform, components::Missile>(
            [&registry, this](Entity entity, components::Transform& transform, components::Missile& /*missile*/) {
                if (transform.x < -100.0f || transform.x > m_screenWidth + 100.0f ||
                    transform.y < -100.0f || transform.y > m_screenHeight + 100.0f) {
                    registry.scheduleDestroy(entity);
                }
            });
    }

    [[nodiscard]] int priority() const override {
        return 25; // Après AnimationSystem
    }

    /**
     * @brief Configure les limites de l'écran
     */
    void setScreenBounds(float width, float height) {
        m_screenWidth = width;
        m_screenHeight = height;
    }

private:
    float m_screenWidth = 1920.0f;
    float m_screenHeight = 1080.0f;
};

} // namespace rtype::ecs::systems
