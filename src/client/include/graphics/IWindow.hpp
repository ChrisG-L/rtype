/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** IWindow
*/

#ifndef IWINDOW_HPP_
#define IWINDOW_HPP_

#include "../utils/Vecs.hpp"
#include "IDrawable.hpp"

#include <string>
#include <any>
#include <optional>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Color.hpp>

namespace graphics {

    class IWindow {
        public:
            virtual ~IWindow() = default;

            virtual bool initialize(Vec2u winSize, const std::string& name) = 0;

            virtual bool isOpen() = 0;
            virtual std::optional<sf::Event> pollEvent() = 0;

            virtual void draw(const sf::Sprite& sprite) = 0;
            virtual void drawRect(float x, float y, float width, float height, sf::Color color) = 0;
            virtual void clear() = 0;
            virtual void display() = 0;

        private:
    };
}

#endif /* !IWINDOW_HPP_ */
