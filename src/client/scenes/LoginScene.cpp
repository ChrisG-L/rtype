/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** LoginScene
*/

#include "scenes/LoginScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/GameScene.hpp"
#include "core/Logger.hpp"

LoginScene::LoginScene()
{
    client::logging::Logger::getSceneLogger()->debug("LoginScene created");
}

void LoginScene::handleEvent(const sf::Event &event)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Space) {
            client::logging::Logger::getSceneLogger()->info("Switching to GameScene");
            if (_sceneManager) {
                _sceneManager->changeScene(std::make_unique<GameScene>());
            }
        }
    }
}

void LoginScene::update()
{
}

void LoginScene::render(std::shared_ptr<graphics::IWindow>  window)
{
    window->drawRect(100.f, 100.f, 200.f, 200.f, sf::Color::Blue);
}
