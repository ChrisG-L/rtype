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

    void Engine::initialize(client::network::TCPClient* tcpClient)
    {
        _tcpClient = tcpClient;

        // Changer le SFML window et rendered par quelque chose de modulaire car couplage trop fort!
        _window = std::make_unique<SFMLWindow>();
        _window->initialize({.x = 1200, .y = 1200}, "rtype");

        _renderer = std::make_unique<SFMLRenderer>(_window.get());
        _renderer->initialize();

        _gameLoop = std::make_unique<GameLoop>(_window.get(), _renderer.get(), _tcpClient);
    }

    void Engine::run()
    {
        _gameLoop->run();
    }
}

