/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Engine
*/

#ifndef ENGINE_HPP_
#define ENGINE_HPP_

#include <dlfcn.h>
#include <memory>

#include "IEngine.hpp"
#include "GameLoop.hpp"
#include "IRenderer.hpp"
#include "DynamicLib.hpp"
#include "../graphics/IWindow.hpp"
#include "../network/UDPClient.hpp"


namespace core {
    class Engine: public IEngine {
        public:
            Engine();
            ~Engine();

            void initialize() override;
            void initialize(
                std::shared_ptr<client::network::UDPClient> udpClient
            );
            void run() override;

        protected:
        private:
            std::shared_ptr<graphics::IWindow>  _window;
            std::unique_ptr<GameLoop> _gameLoop;
            std::unique_ptr<DynamicLib> _dynamicLib;
            std::shared_ptr<client::network::UDPClient> _udpClient = nullptr;
            graphics::IGraphicPlugin* _graphicPlugin = nullptr;
    };
}

#endif /* !ENGINE_HPP_ */
