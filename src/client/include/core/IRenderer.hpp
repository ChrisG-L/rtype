/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** IRenderer
*/

#ifndef IRENDERER_HPP_
#define IRENDERER_HPP_

#include "../graphics/IWindow.hpp"
#include <SFML/Graphics.hpp>

namespace core {
    class IRenderer {
        public:
            virtual ~IRenderer() = default;

            virtual void initialize() = 0;
            virtual void update() = 0;

    };
}

#endif /* !IRENDERER_HPP_ */
