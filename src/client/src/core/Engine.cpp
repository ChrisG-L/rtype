/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Engine
*/

#include "core/Engine.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include "core/DynamicLib.hpp"
#include "graphics/Graphics.hpp"
#include "graphics/IGraphicPlugin.hpp"
#include "network/UDPClient.hpp"

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

    void Engine::initialize(
        std::shared_ptr<client::network::UDPClient> udpClient
    )
    {
        _udpClient = udpClient;

        #ifdef _WIN32
            _graphicPlugin = _dynamicLib->openGraphicLib("rtype_sdl2.dll");
        #else
            _graphicPlugin = _dynamicLib->openGraphicLib("librtype_sdl2.so");
        #endif
        _window = _graphicPlugin->createWindow({.x = 1920, .y = 1080}, "rtype");
        _gameLoop = std::make_unique<GameLoop>(_window, _graphicPlugin, _udpClient);
    }

    void Engine::run()
    {
        _gameLoop->run();
    }
}
