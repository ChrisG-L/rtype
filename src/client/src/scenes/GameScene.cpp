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
#include "events/Event.hpp"
#include <variant>

GameScene::GameScene()
{
    client::logging::Logger::getSceneLogger()->debug("GameScene created");
}

void GameScene::handleEvent(const events::Event &event)
{
    if (std::holds_alternative<events::KeyPressed>(event)) {
        const auto& keyPressed = std::get<events::KeyPressed>(event);
        if (keyPressed.key == events::Key::Space) {
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
