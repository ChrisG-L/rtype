/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** AnimationSystem - Système d'animation
*/

#pragma once

#include "../System.hpp"
#include "../Registry.hpp"
#include "../components/Animation.hpp"
#include "../components/Sprite.hpp"

namespace rtype::ecs::systems {

/**
 * @brief Système d'animation des sprites
 *
 * Met à jour les animations et synchronise le sprite avec la frame actuelle.
 */
class AnimationSystem : public ISystem {
public:
    AnimationSystem() = default;

    void update(Registry& registry, float dt) override {
        registry.view<components::Animation, components::Sprite>(
            [dt](Entity /*entity*/, components::Animation& anim, components::Sprite& sprite) {
                if (!anim.playing || anim.frames.empty()) return;

                // Vérifie que frameTime est valide pour éviter une boucle infinie
                if (anim.frameTime <= 0.0f) {
                    anim.frameTime = 0.1f;  // Valeur par défaut sûre
                }

                anim.currentTime += dt;

                // Passe à la frame suivante si nécessaire
                while (anim.currentTime >= anim.frameTime) {
                    anim.currentTime -= anim.frameTime;
                    anim.currentFrame++;

                    // Gère la fin de l'animation
                    if (anim.currentFrame >= anim.frames.size()) {
                        if (anim.loop) {
                            anim.currentFrame = 0;
                        } else {
                            anim.currentFrame = anim.frames.size() > 0 ? anim.frames.size() - 1 : 0;
                            anim.playing = false;
                        }
                    }
                }

                // Met à jour le rectangle du sprite (avec vérification de sécurité)
                if (anim.currentFrame < anim.frames.size()) {
                    sprite.textureRect = anim.frames[anim.currentFrame];
                }
            });
    }

    [[nodiscard]] int priority() const override {
        return 20; // Après MovementSystem
    }
};

} // namespace rtype::ecs::systems
