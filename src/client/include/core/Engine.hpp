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
#include "../implementations/sfml/SFMLRenderer.hpp"

#include <memory>

namespace core {
    class Engine: public IEngine {
        public:
            Engine();
            ~Engine();

            void initialize() override;
            void run() override;

        protected:
        private:
            std::unique_ptr<graphics::IWindow> _window;
            std::unique_ptr<IRenderer> _renderer;
            std::unique_ptr<GameLoop> _gameLoop;
    };
}

#endif /* !ENGINE_HPP_ */
