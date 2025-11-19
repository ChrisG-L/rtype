/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** InputSystem - Système de gestion des entrées
*/

#pragma once

#include "../System.hpp"
#include "../Registry.hpp"
#include "../components/PlayerInput.hpp"
#include "../components/NetworkId.hpp"
#include <SFML/Window/Keyboard.hpp>

namespace rtype::ecs::systems {

/**
 * @brief Système de gestion des entrées clavier
 *
 * Capture les entrées du joueur local et met à jour son composant PlayerInput.
 * Les inputs sont ensuite envoyés au serveur par le NetworkSystem.
 */
class InputSystem : public ISystem {
public:
    InputSystem() = default;

    void update(Registry& registry, float /*dt*/) override {
        // Itère sur toutes les entités avec PlayerInput et NetworkId
        registry.view<components::PlayerInput, components::NetworkId>(
            [](Entity /*entity*/, components::PlayerInput& input, components::NetworkId& netId) {
                // Ne traite que le joueur local
                if (!netId.isLocalPlayer) return;

                // Capture les entrées clavier
                input.up    = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z) ||
                              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
                input.down  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
                input.left  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q) ||
                              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
                input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
                              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);
                input.shoot = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);

                // Incrémente le numéro de séquence si input actif
                if (input.hasInput()) {
                    input.sequenceNumber++;
                }
            });
    }

    [[nodiscard]] int priority() const override {
        return 0; // Premier système à s'exécuter
    }
};

} // namespace rtype::ecs::systems
