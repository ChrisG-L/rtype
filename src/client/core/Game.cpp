/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Game - Implémentation du game loop
*/

#include "Game.hpp"
#include <SFML/Window/VideoMode.hpp>

namespace rtype::core {

Game::Game(const GameConfig& config)
    : m_config(config)
    , m_fixedDeltaTime(1.0f / config.tickRate)
{
    // Crée la fenêtre SFML
    sf::VideoMode mode({config.width, config.height});

    if (config.fullscreen) {
        m_window.create(mode, config.title, sf::State::Fullscreen);
    } else {
        m_window.create(mode, config.title);
    }

    // Configure VSync
    m_window.setVerticalSyncEnabled(config.vsync);

    // Initialise les systèmes
    initSystems();
}

void Game::run() {
    m_running = true;

    // Crée les entités initiales
    createEntities();

    // Lance la boucle de jeu
    gameLoop();
}

void Game::stop() {
    m_running = false;
}

void Game::initSystems() {
    // Ajoute les systèmes dans l'ordre de priorité
    m_registry.addSystem<ecs::systems::InputSystem>();
    m_movementSystem = &m_registry.addSystem<ecs::systems::MovementSystem>();
    m_interpolationSystem = &m_registry.addSystem<ecs::systems::InterpolationSystem>();
    m_registry.addSystem<ecs::systems::AnimationSystem>();
    m_registry.addSystem<ecs::systems::MissileSystem>();
    m_collisionSystem = &m_registry.addSystem<ecs::systems::CollisionSystem>();
    m_renderSystem = &m_registry.addSystem<ecs::systems::RenderSystem>();

    // Configure les limites d'écran
    m_movementSystem->setScreenBounds(
        static_cast<float>(m_config.width),
        static_cast<float>(m_config.height)
    );
}

void Game::createEntities() {
    // À surcharger par les classes dérivées
}

void Game::handleEvent(const sf::Event& /*event*/) {
    // À surcharger par les classes dérivées
}

void Game::fixedUpdate(float /*dt*/) {
    // À surcharger par les classes dérivées
}

void Game::update(float /*dt*/) {
    // À surcharger par les classes dérivées
}

void Game::render() {
    // Efface l'écran
    m_window.clear(sf::Color::Black);

    // Rendu des entités ECS
    m_renderSystem->render(m_registry, m_window);

    // Affiche
    m_window.display();
}

void Game::processEvents() {
    while (const std::optional event = m_window.pollEvent()) {
        // Fermeture de la fenêtre
        if (event->is<sf::Event::Closed>()) {
            stop();
            return;
        }

        // Échap pour quitter
        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                stop();
                return;
            }
        }

        // Traitement personnalisé
        handleEvent(*event);
    }
}

void Game::gameLoop() {
    m_clock.restart();

    while (m_running && m_window.isOpen()) {
        // Calcule le delta time
        float frameTime = m_clock.restart();

        // Limite le frame time pour éviter la spirale de la mort
        if (frameTime > 0.25f) {
            frameTime = 0.25f;
        }

        // Traite les événements
        processEvents();

        // Accumule le temps pour le fixed update
        m_accumulator += frameTime;

        // Fixed timestep loop
        while (m_accumulator >= m_fixedDeltaTime) {
            // Mise à jour logique à taux fixe
            m_registry.update(m_fixedDeltaTime);
            fixedUpdate(m_fixedDeltaTime);

            // Traite les destructions en attente
            m_registry.processDestructions();

            m_accumulator -= m_fixedDeltaTime;
        }

        // Mise à jour variable (interpolation visuelle, etc.)
        update(frameTime);

        // Rendu
        render();
    }

    m_window.close();
}

} // namespace rtype::core
