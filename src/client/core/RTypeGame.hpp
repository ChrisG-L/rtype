/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** RTypeGame - Classe principale du jeu R-Type
*/

#pragma once

#include "Game.hpp"
#include "../graphics/AssetManager.hpp"
#include "../graphics/Starfield.hpp"
#include "../network/NetworkSystem.hpp"

namespace rtype::core {

/**
 * @brief Classe principale du jeu R-Type
 *
 * Hérite de Game et ajoute la logique spécifique au R-Type :
 * starfield, assets, connexion réseau, etc.
 */
class RTypeGame : public Game {
public:
    explicit RTypeGame(const GameConfig& config = GameConfig());

    /**
     * @brief Connecte au serveur de jeu
     */
    bool connectToServer(const std::string& host, std::uint16_t port, const std::string& playerName);

    /**
     * @brief Accès à l'AssetManager
     */
    [[nodiscard]] graphics::AssetManager& getAssets() { return m_assetManager; }

    /**
     * @brief Accès au système réseau
     */
    [[nodiscard]] network::NetworkSystem* getNetworkSystem() { return m_networkSystem; }

protected:
    void initSystems() override;
    void createEntities() override;
    void handleEvent(const sf::Event& event) override;
    void fixedUpdate(float dt) override;
    void update(float dt) override;
    void render() override;

private:
    void loadAssets();
    void createLocalPlayer();

private:
    graphics::AssetManager m_assetManager;
    graphics::Starfield m_starfield;

    network::NetworkSystem* m_networkSystem = nullptr;

    bool m_connected = false;
};

} // namespace rtype::core
