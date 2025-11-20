/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** GameScene
*/

#include "scenes/GameScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/LoginScene.hpp"

GameScene::GameScene()
{
    std::cout << "[GameScene] Created" << std::endl;
}

void GameScene::handleEvent(const sf::Event &event)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Space) {
            std::cout << "[GameScene] Switching to LoginScene" << std::endl;
            if (_sceneManager) {
                _sceneManager->changeScene(std::make_unique<LoginScene>());
            }
        }
    }
}

void GameScene::update()
{
}

void GameScene::render(graphics::IWindow *window)
{
    window->drawRect(100.f, 100.f, 200.f, 200.f, sf::Color::Red);
}
