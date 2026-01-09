/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Engine
*/

#include "core/Engine.hpp"
#include "boot/Boot.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include "core/DynamicLib.hpp"
#include "core/Logger.hpp"
#include "accessibility/ColorblindShaderManager.hpp"
#include "graphics/Graphics.hpp"
#include "graphics/IGraphicPlugin.hpp"
#include "network/UDPClient.hpp"
#include "network/TCPClient.hpp"

namespace core {
    Engine::Engine(): _dynamicLib(std::make_unique<DynamicLib>())
    {
    }

    Engine::~Engine()
    {
        // Release window reference before unloading plugin to avoid use-after-free
        accessibility::ColorblindShaderManager::getInstance().shutdown();

        if (_graphicPlugin && _dynamicLib) {
            _dynamicLib->destroyGraphicLib(_graphicPlugin);
        }
    }

    void Engine::initialize()
    {
        GraphicsOptions defaultOptions;
        initialize(nullptr, nullptr, defaultOptions);
    }

    std::string Engine::buildLibraryName(const std::string& name) const
    {
        #ifdef _WIN32
            return "rtype_" + name + ".dll";
        #else
            return "librtype_" + name + ".so";
        #endif
    }

    void Engine::initialize(
        std::shared_ptr<client::network::UDPClient> udpClient,
        std::shared_ptr<client::network::TCPClient> tcpClient,
        const GraphicsOptions& graphicsOptions
    )
    {
        auto logger = client::logging::Logger::getEngineLogger();
        _udpClient = udpClient;
        _tcpClient = tcpClient;

        std::string libPath;

        // Priority: --graphics-path > --graphics > default (sfml)
        if (graphicsOptions.path.has_value()) {
            libPath = graphicsOptions.path.value();
            logger->info("Using custom graphics plugin path: {}", libPath);
        } else if (graphicsOptions.name.has_value()) {
            libPath = buildLibraryName(graphicsOptions.name.value());
            logger->info("Using graphics backend '{}': {}", graphicsOptions.name.value(), libPath);
        } else {
            libPath = buildLibraryName("sfml");
            logger->info("Using default graphics backend (SFML): {}", libPath);
        }

        try {
            _graphicPlugin = _dynamicLib->openGraphicLib(libPath);
        } catch (const std::exception& e) {
            std::cerr << "\n[ERROR] Failed to load graphics plugin: " << libPath << "\n"
                      << "        " << e.what() << "\n\n"
                      << "Available options:\n"
                      << "  --graphics=<name>       Use a named backend (e.g., sdl2, sfml)\n"
                      << "  --graphics-path=<path>  Use a custom plugin path\n\n"
                      << "Examples:\n"
                      << "  ./rtype_client --graphics=sfml\n"
                      << "  ./rtype_client --graphics-path=/path/to/custom_plugin.so\n";
            throw;
        }

        _window = _graphicPlugin->createWindow({.x = 1920, .y = 1080}, "rtype");
        _gameLoop = std::make_unique<GameLoop>(_window, _graphicPlugin, _udpClient, _tcpClient);
    }

    void Engine::run()
    {
        _gameLoop->run();
    }
}
