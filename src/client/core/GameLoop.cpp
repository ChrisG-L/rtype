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
    }

    GameLoop::~GameLoop()
    {
    }

    void GameLoop::run()
    {
        while (_window->isOpen()) {
            clear();
            _renderer->update();
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
