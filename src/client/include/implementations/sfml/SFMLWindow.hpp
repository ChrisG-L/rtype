/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** SFMLWindow
*/

#ifndef SFMLWINDOW_HPP_
#define SFMLWINDOW_HPP_

#include "../../graphics/IWindow.hpp"

#include <SFML/Graphics.hpp>

class SFMLWindow: public graphics::IWindow {
    public:
        bool initialize(Vec2u winSize, const std::string& name) override;
        bool isOpen() override;
        std::optional<sf::Event> pollEvent() override;
        void draw(const sf::Sprite& sprite) override;
        void drawRect(float x, float y, float width, float height, sf::Color color) override;
        void clear() override;
        void display() override;

    private:
        sf::RenderWindow _window;
};

#endif /* !SFMLWINDOW_HPP_ */
