/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Engine
*/

#ifndef ENGINE_HPP_
#define ENGINE_HPP_

#include <memory>
#include <string>
#include <optional>

#include "IEngine.hpp"
#include "GameLoop.hpp"
#include "IRenderer.hpp"
#include "DynamicLib.hpp"
#include "../graphics/IWindow.hpp"
#include "../network/UDPClient.hpp"
#include "../network/TCPClient.hpp"

struct GraphicsOptions;

namespace core {
    class Engine: public IEngine {
        public:
            Engine();
            ~Engine();

            void initialize() override;
            void initialize(
                std::shared_ptr<client::network::UDPClient> udpClient,
                std::shared_ptr<client::network::TCPClient> tcpClient,
                const GraphicsOptions& graphicsOptions
            );
            void run() override;

        protected:
        private:
            std::string buildLibraryName(const std::string& name) const;

            std::shared_ptr<graphics::IWindow>  _window;
            std::unique_ptr<GameLoop> _gameLoop;
            std::unique_ptr<DynamicLib> _dynamicLib;
            std::shared_ptr<client::network::UDPClient> _udpClient = nullptr;
            std::shared_ptr<client::network::TCPClient> _tcpClient = nullptr;
            graphics::IGraphicPlugin* _graphicPlugin = nullptr;
    };
}

#endif /* !ENGINE_HPP_ */
