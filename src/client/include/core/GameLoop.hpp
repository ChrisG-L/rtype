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

#include <memory>

namespace core {
    class GameLoop: public IGameLoop {
        public:
            GameLoop(graphics::IWindow* window, IRenderer* renderer);
            ~GameLoop();

            void run() override;
            void clear() override;
            void display() override;

        private:
            graphics::IWindow* _window;
            IRenderer* _renderer;
            std::unique_ptr<SceneManager> _sceneManager;
    };
}

#endif /* !GAMELOOP_HPP_ */
