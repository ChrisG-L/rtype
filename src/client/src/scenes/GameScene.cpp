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

void GameScene::handleEvent(const events::Event& event)
{
    if (std::holds_alternative<events::KeyPressed>(event)) {
        auto& key = std::get<events::KeyPressed>(event);
        if (key.key == events::Key::Down) {
            std::cout << "Game scene Key down ! " << std::endl;
        }
    }
}

void GameScene::update(float deltatime)
{
}

void GameScene::render()
{
}
