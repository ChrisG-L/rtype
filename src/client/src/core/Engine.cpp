/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Engine
*/

#include "core/Engine.hpp"

namespace core {
    Engine::Engine()
    {
    }

    Engine::~Engine()
    {
    }

    void Engine::initialize()
    {
        initialize(nullptr);
    }

    void Engine::initialize(std::shared_ptr<client::network::TCPClient> tcpClient)
    {
        _tcpClient = tcpClient;

        // Changer le SFML window et rendered par quelque chose de modulaire car couplage trop fort!
        // _window = std::make_shared<SFMLWindow>(); // Changer ceci
        // _window->initialize({.x = 1920, .y = 1080}, "rtype");

        _gameLoop = std::make_unique<GameLoop>(_window, _tcpClient);
    }

    void Engine::run()
    {
        _gameLoop->run();
    }
}

