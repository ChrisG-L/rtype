/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** RTypeGame - Implémentation de la classe principale
*/

#include "RTypeGame.hpp"
#include <iostream>

namespace rtype::core {

RTypeGame::RTypeGame(const GameConfig& config)
    : Game(config)
    , m_starfield(static_cast<float>(config.width),
                  static_cast<float>(config.height), 300)
{
    // Charge les assets
    loadAssets();
}

void RTypeGame::initSystems() {
    // Appelle l'initialisation de base
    Game::initSystems();

    // Ajoute le système réseau
    m_networkSystem = &getRegistry().addSystem<network::NetworkSystem>();
}

void RTypeGame::createEntities() {
    // Crée le joueur local (en mode hors-ligne pour test)
    if (!m_connected) {
        createLocalPlayer();
    }
}

void RTypeGame::handleEvent(const sf::Event& /*event*/) {
    // Gestion des événements spécifiques
}

void RTypeGame::fixedUpdate(float /*dt*/) {
    // Mise à jour logique fixe
}

void RTypeGame::update(float dt) {
    // Met à jour le starfield
    m_starfield.update(dt);
}

void RTypeGame::render() {
    // Efface avec un fond noir
    getWindow().clear(sf::Color(10, 10, 20));

    // Dessine le starfield en arrière-plan
    m_starfield.draw(getWindow());

    // Rendu des entités ECS
    getRenderSystem().render(getRegistry(), getWindow());

    // Affiche
    getWindow().display();
}

bool RTypeGame::connectToServer(const std::string& host, std::uint16_t port, const std::string& playerName) {
    if (!m_networkSystem) {
        std::cerr << "Système réseau non initialisé!" << std::endl;
        return false;
    }

    if (m_networkSystem->connect(host, port, playerName)) {
        m_connected = true;
        return true;
    }

    return false;
}

void RTypeGame::loadAssets() {
    // Configure le chemin de base
    m_assetManager.setBasePath("assets");

    // Charge les textures (à adapter selon vos fichiers)
    // m_assetManager.loadTexture("player", "sprites/player.png");
    // m_assetManager.loadTexture("enemy", "sprites/enemy.png");
    // m_assetManager.loadTexture("missile", "sprites/missile.png");

    // Pour l'instant, on va dessiner des rectangles colorés

    std::cout << "Assets chargés!" << std::endl;
}

void RTypeGame::createLocalPlayer() {
    auto& registry = getRegistry();

    // Crée le joueur local
    ecs::Entity player = registry.createEntity();

    // Transform (position initiale au centre-gauche)
    registry.addComponent<ecs::components::Transform>(
        player,
        100.0f,
        static_cast<float>(getConfig().height) / 2.0f
    );

    // Velocity
    registry.addComponent<ecs::components::Velocity>(player);

    // NetworkId (local)
    registry.addComponent<ecs::components::NetworkId>(
        player,
        0, // ID temporaire
        ecs::components::NetworkEntityType::Player,
        true // isLocalPlayer
    );

    // PlayerInput
    registry.addComponent<ecs::components::PlayerInput>(player);

    // Health
    registry.addComponent<ecs::components::Health>(player, 100);

    // Sprite (rectangle pour test)
    auto& sprite = registry.addComponent<ecs::components::Sprite>(player, "player", 10);
    sprite.textureRect = sf::IntRect({0, 0}, {64, 64});
    sprite.color = sf::Color::Cyan;

    // Collider
    registry.addComponent<ecs::components::Collider>(
        player,
        64.0f, 64.0f,
        ecs::components::CollisionLayer::Player
    );

    std::cout << "Joueur local créé (ID: " << player << ")" << std::endl;
}

} // namespace rtype::core
