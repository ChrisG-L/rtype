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
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

GameScene::GameScene()
{
    client::logging::Logger::getSceneLogger()->debug("GameScene created");
}

void GameScene::handleEvent(const sf::Event &event)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Space) {
            client::logging::Logger::getSceneLogger()->info("Switching to LoginScene");
            // if (_sceneManager) {
            //     _sceneManager->changeScene(std::make_unique<LoginScene>());
            // }
        }
    }
}

void GameScene::update(float deltatime)
{
}

void GameScene::render()
{
}
