/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** GameLoop
*/

#include "core/GameLoop.hpp"
#include <iostream>

namespace core {
    GameLoop::GameLoop(graphics::IWindow* window, IRenderer* renderer): _window(window), _renderer(renderer)
    {
        _sceneManager = std::make_unique<SceneManager>();
        _sceneManager->changeScene(std::make_unique<LoginScene>());
    }

    GameLoop::~GameLoop()
    {
    }

    void GameLoop::run()
    {
        while (_window->isOpen()) {
            while (auto event = _window->pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    return;
                }
                _sceneManager->handleEvent(*event);
            }

            // Update
            _sceneManager->update();

            // Render
            clear();
            _sceneManager->render(_window);
            display();
        }
    }

    void GameLoop::clear()
    {
        _window->clear();
    }

    void GameLoop::display()
    {
        _window->display();
    }

}
