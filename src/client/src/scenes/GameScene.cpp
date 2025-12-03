/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** GameScene
*/

#include "scenes/GameScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/LoginScene.hpp"
#include "core/Logger.hpp"

GameScene::GameScene()
{
    client::logging::Logger::getSceneLogger()->debug("GameScene created");
}

void GameScene::handleEvent(const events::Event& event)
{
    // if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
    //     if (keyPressed->code == sf::Keyboard::Key::Space) {
    //         client::logging::Logger::getSceneLogger()->info("Switching to LoginScene");
    //         // if (_sceneManager) {
    //         //     _sceneManager->changeScene(std::make_unique<LoginScene>());
    //         // }
    //     }
    // }
    if (std::holds_alternative<events::KeyPressed>(event)) {
        auto& key = std::get<events::KeyPressed>(event);
        if (key.key == events::Key::Down) {
            std::cout << "Game scene Key down ! " << std::endl;
            // _udpClient->movePlayer(10, 15);
        }
    }

}

void GameScene::update(float deltatime)
{
}

void GameScene::render()
{
}
