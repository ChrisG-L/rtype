/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** IGameLoop
*/

#ifndef IGAMELOOP_HPP_
#define IGAMELOOP_HPP_

#include "IRenderer.hpp"
#include "../graphics/IWindow.hpp"
#include "../graphics/IDrawable.hpp"

#include <memory>

namespace core {
    class IGameLoop {
        public:
            virtual ~IGameLoop() = default;

            virtual void run() = 0;
            virtual void clear() = 0;
            virtual void display() = 0;

        protected:
        private:
    };
}
#endif /* !IGAMELOOP_HPP_ */
