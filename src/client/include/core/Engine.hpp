/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Engine
*/

#ifndef ENGINE_HPP_
#define ENGINE_HPP_

#include "IEngine.hpp"
#include "GameLoop.hpp"
#include "IRenderer.hpp"
#include "../graphics/IWindow.hpp"
#include "../implementations/sfml/SFMLWindow.hpp"
#include "../network/TCPClient.hpp"

#include <memory>

namespace core {
    class Engine: public IEngine {
        public:
            Engine();
            ~Engine();

            void initialize() override;
            void initialize(std::shared_ptr<client::network::TCPClient> tcpClient);
            void run() override;

        protected:
        private:
            std::shared_ptr<graphics::IWindow>  _window;
            std::unique_ptr<GameLoop> _gameLoop;
            std::shared_ptr<client::network::TCPClient> _tcpClient = nullptr;
    };
}

#endif /* !ENGINE_HPP_ */
