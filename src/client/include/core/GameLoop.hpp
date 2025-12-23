/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** GameLoop
*/

#ifndef GAMELOOP_HPP_
#define GAMELOOP_HPP_

#include <memory>
#include <iostream>
#include <vector>
#include <chrono>

#include "scenes/SceneManager.hpp"
#include "network/UDPClient.hpp"
#include "network/TCPClient.hpp"
#include "IGameLoop.hpp"
#include "core/IRenderer.hpp"
#include "events/Event.hpp"
#include "events/Signal.hpp"
#include "../graphics/Graphics.hpp"
#include "graphics/IGraphicPlugin.hpp"

namespace core {
    class GameLoop: public IGameLoop {
        public:
            GameLoop(
                std::shared_ptr<graphics::IWindow> window,
                graphics::IGraphicPlugin* _graphicPlugin,
                std::shared_ptr<client::network::UDPClient> udpClient = nullptr,
                std::shared_ptr<client::network::TCPClient> tcpClient = nullptr
            );
            ~GameLoop();

            void run() override;
            void clear() override;
            void display() override;

        private:
            float _deltatime;
            std::shared_ptr<graphics::IWindow> _window;
            std::shared_ptr<IRenderer> _renderer;
            std::unique_ptr<SceneManager> _sceneManager;
            std::shared_ptr<client::network::UDPClient> _udpClient;
            std::shared_ptr<client::network::TCPClient> _tcpClient;
    };
}

#endif /* !GAMELOOP_HPP_ */
