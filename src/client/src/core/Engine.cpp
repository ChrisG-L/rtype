/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Engine
*/

#include "core/Engine.hpp"
#include <dlfcn.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include "core/DynamicLib.hpp"
#include "graphics/Graphics.hpp"
#include "graphics/IGraphicPlugin.hpp"

namespace core {
    Engine::Engine(): _dynamicLib(std::make_unique<DynamicLib>())
    {
    }

    Engine::~Engine()
    {
        if (_graphicPlugin && _dynamicLib) {
            _dynamicLib->destroyGraphicLib(_graphicPlugin);
        }
    }

    void Engine::initialize()
    {
        initialize(nullptr);
    }

    void Engine::initialize(std::shared_ptr<client::network::TCPClient> tcpClient)
    {
        _tcpClient = tcpClient;

        _graphicPlugin = _dynamicLib->openGraphicLib("librtype_sfml.so");
        std::cout << _graphicPlugin->getName() << std::endl;
        _window = _graphicPlugin->createWindow({.x = 1920, .y = 1080}, "rtype");
        _gameLoop = std::make_unique<GameLoop>(_window, _graphicPlugin, _tcpClient);
    }

    void Engine::run()
    {
        _gameLoop->run();
    }
}

