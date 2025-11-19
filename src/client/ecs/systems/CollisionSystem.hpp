/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** CollisionSystem - Système de détection de collisions
*/

#pragma once

#include "../System.hpp"
#include "../Registry.hpp"
#include "../components/Transform.hpp"
#include "../components/Collider.hpp"
#include "../components/Health.hpp"
#include "../components/Missile.hpp"
#include <functional>
#include <vector>

namespace rtype::ecs::systems {

/**
 * @brief Paire de collision
 */
struct CollisionPair {
    Entity a;
    Entity b;
};

/**
 * @brief Système de détection de collisions
 *
 * Détecte les collisions entre entités et applique les effets.
 * Note: Dans un modèle client/serveur autoritatif, les collisions
 * importantes sont validées par le serveur.
 */
class CollisionSystem : public ISystem {
public:
    using CollisionCallback = std::function<void(Entity, Entity)>;

    CollisionSystem() = default;

    void update(Registry& registry, float dt) override {
        m_collisions.clear();

        // Récupère toutes les entités avec collision
        auto& entities = registry.getEntitiesWith<components::Collider>();

        // Test de collision O(n²) - acceptable pour un shoot'em up
        for (std::size_t i = 0; i < entities.size(); ++i) {
            Entity a = entities[i];
            auto* transformA = registry.tryGetComponent<components::Transform>(a);
            auto* colliderA = registry.tryGetComponent<components::Collider>(a);

            if (!transformA || !colliderA) continue;

            for (std::size_t j = i + 1; j < entities.size(); ++j) {
                Entity b = entities[j];
                auto* transformB = registry.tryGetComponent<components::Transform>(b);
                auto* colliderB = registry.tryGetComponent<components::Collider>(b);

                if (!transformB || !colliderB) continue;

                // Vérifie les masques de collision
                if (!shouldCollide(*colliderA, *colliderB)) continue;

                // Test AABB
                if (checkAABB(*transformA, *colliderA, *transformB, *colliderB)) {
                    m_collisions.push_back({a, b});

                    // Appelle le callback si défini
                    if (m_callback) {
                        m_callback(a, b);
                    }
                }
            }
        }

        // Met à jour les timers d'invincibilité
        registry.view<components::Health>(
            [dt](Entity /*entity*/, components::Health& health) {
                if (health.invincible && health.invincibleTimer > 0.0f) {
                    health.invincibleTimer -= dt;
                    if (health.invincibleTimer <= 0.0f) {
                        health.invincible = false;
                    }
                }
            });
    }

    [[nodiscard]] int priority() const override {
        return 30; // Après AnimationSystem
    }

    /**
     * @brief Définit le callback de collision
     */
    void setCollisionCallback(CollisionCallback callback) {
        m_callback = std::move(callback);
    }

    /**
     * @brief Retourne les collisions détectées cette frame
     */
    [[nodiscard]] const std::vector<CollisionPair>& getCollisions() const {
        return m_collisions;
    }

private:
    /**
     * @brief Vérifie si deux colliders doivent tester la collision
     */
    [[nodiscard]] bool shouldCollide(const components::Collider& a, const components::Collider& b) const {
        // Vérifie si les masques correspondent
        std::uint8_t layerA = static_cast<std::uint8_t>(a.layer);
        std::uint8_t layerB = static_cast<std::uint8_t>(b.layer);

        return (layerA & b.mask) || (layerB & a.mask);
    }

    /**
     * @brief Test de collision AABB
     */
    [[nodiscard]] bool checkAABB(
        const components::Transform& tA, const components::Collider& cA,
        const components::Transform& tB, const components::Collider& cB) const
    {
        float ax = tA.x + cA.offsetX;
        float ay = tA.y + cA.offsetY;
        float bx = tB.x + cB.offsetX;
        float by = tB.y + cB.offsetY;

        return ax < bx + cB.width &&
               ax + cA.width > bx &&
               ay < by + cB.height &&
               ay + cA.height > by;
    }

private:
    std::vector<CollisionPair> m_collisions;
    CollisionCallback m_callback;
};

} // namespace rtype::ecs::systems
