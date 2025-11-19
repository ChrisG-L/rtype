/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Game - Classe principale du jeu avec game loop
*/

#pragma once

#include <memory>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include "../ecs/Registry.hpp"
#include "../ecs/systems/Systems.hpp"
#include "../ecs/components/Components.hpp"
#include "Clock.hpp"

namespace rtype::core {

/**
 * @brief Configuration du jeu
 */
struct GameConfig {
    std::string title = "R-Type";
    unsigned int width = 1920;
    unsigned int height = 1080;
    bool fullscreen = false;
    bool vsync = true;
    float tickRate = 60.0f;  // Taux de mise à jour logique (Hz)
};

/**
 * @brief Classe principale du jeu
 *
 * Gère le game loop avec fixed timestep, la fenêtre SFML,
 * et coordonne le registre ECS.
 */
class Game {
public:
    explicit Game(const GameConfig& config = GameConfig());
    ~Game() = default;

    // Non-copiable
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    /**
     * @brief Lance le jeu
     */
    void run();

    /**
     * @brief Arrête le jeu
     */
    void stop();

    /**
     * @brief Accès au registre ECS
     */
    [[nodiscard]] ecs::Registry& getRegistry() { return m_registry; }
    [[nodiscard]] const ecs::Registry& getRegistry() const { return m_registry; }

    /**
     * @brief Accès à la fenêtre
     */
    [[nodiscard]] sf::RenderWindow& getWindow() { return m_window; }

    /**
     * @brief Accès au système de rendu
     */
    [[nodiscard]] ecs::systems::RenderSystem& getRenderSystem() { return *m_renderSystem; }

    /**
     * @brief Retourne la configuration
     */
    [[nodiscard]] const GameConfig& getConfig() const { return m_config; }

protected:
    /**
     * @brief Initialise les systèmes (à surcharger)
     */
    virtual void initSystems();

    /**
     * @brief Crée les entités initiales (à surcharger)
     */
    virtual void createEntities();

    /**
     * @brief Traite un événement (à surcharger)
     */
    virtual void handleEvent(const sf::Event& event);

    /**
     * @brief Mise à jour avec dt fixe (à surcharger)
     */
    virtual void fixedUpdate(float dt);

    /**
     * @brief Mise à jour avant le rendu (à surcharger)
     */
    virtual void update(float dt);

    /**
     * @brief Rendu (à surcharger)
     */
    virtual void render();

private:
    void processEvents();
    void gameLoop();

private:
    GameConfig m_config;
    sf::RenderWindow m_window;
    ecs::Registry m_registry;
    Clock m_clock;

    // Pointeurs vers les systèmes pour un accès rapide
    ecs::systems::RenderSystem* m_renderSystem = nullptr;
    ecs::systems::MovementSystem* m_movementSystem = nullptr;
    ecs::systems::CollisionSystem* m_collisionSystem = nullptr;
    ecs::systems::InterpolationSystem* m_interpolationSystem = nullptr;

    bool m_running = false;
    float m_accumulator = 0.0f;
    float m_fixedDeltaTime = 1.0f / 60.0f;
};

} // namespace rtype::core
