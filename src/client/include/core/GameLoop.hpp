/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** GameLoop
*/

#ifndef GAMELOOP_HPP_
#define GAMELOOP_HPP_

#include "IGameLoop.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/LoginScene.hpp"
#include "network/TCPClient.hpp"

#include <memory>

namespace core {
    class GameLoop: public IGameLoop {
        public:
            GameLoop(
                std::shared_ptr<graphics::IWindow> window,
                std::shared_ptr<IRenderer>,
                std::shared_ptr<client::network::TCPClient> tcpClient = nullptr);
            ~GameLoop();

            void run() override;
            void clear() override;
            void display() override;

        private:
            std::shared_ptr<graphics::IWindow> _window;
            std::shared_ptr<IRenderer> _renderer;
            std::unique_ptr<SceneManager> _sceneManager;
            std::shared_ptr<client::network::TCPClient> _tcpClient;
    };
}

#endif /* !GAMELOOP_HPP_ */
