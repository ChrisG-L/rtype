/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** SFMLWindow
*/

#ifndef SFMLWINDOW_HPP_
#define SFMLWINDOW_HPP_

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>

#include <iostream>

#include "graphics/IWindow.hpp"
#include "graphics/IDrawable.hpp"
#include "utils/Vecs.hpp"

class SFMLWindow: public graphics::IWindow {
    public:
        SFMLWindow(Vec2u winSize, const std::string& name);
        Vec2u getSize() const override;
        bool isOpen() override;
        void close() override;
        bool pollEvent() override;

        void draw(const graphics::IDrawable& drawable) override;
        void drawRect(float x, float y, float width, float height, rgba color) override;
        void drawImg(graphics::IDrawable, float x, float y, float scaleX, float scaleY) override;

        void clear() override;
        void display() override;

    private:
        sf::RenderWindow _window;
};

#endif /* !SFMLWINDOW_HPP_ */
