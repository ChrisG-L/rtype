/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** IWindow
*/

#ifndef IWINDOW_HPP_
#define IWINDOW_HPP_

#include <string>
#include <optional>

#include "../utils/Vecs.hpp"
#include "IDrawable.hpp"
#include "../events/Event.hpp"

namespace graphics {

    class IWindow {
        public:
            virtual ~IWindow() = default;

            virtual Vec2u getSize() const = 0;
            virtual bool isOpen() = 0;
            virtual void close() = 0;
            virtual bool pollEvent() = 0;

            virtual void draw(const IDrawable& drawable) = 0;
            virtual void drawRect(float x, float y, float width, float height, rgba color) = 0;
            virtual void drawImg(graphics::IDrawable, float x, float y, float scaleX, float scaleY) = 0;
            virtual void clear() = 0;
            virtual void display() = 0;

        private:
    };
}

#endif /* !IWINDOW_HPP_ */
