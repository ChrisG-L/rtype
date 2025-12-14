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
#include <unordered_map>

#include "graphics/IWindow.hpp"
#include "graphics/IDrawable.hpp"
#include "utils/Vecs.hpp"

class SFMLWindow: public graphics::IWindow {
    public:
        SFMLWindow(Vec2u winSize, const std::string& name);
        Vec2u getSize() const override;
        bool isOpen() override;
        void close() override;
        events::Event pollEvent() override;

        void draw(const graphics::IDrawable& drawable) override;
        void draw(sf::Sprite& sprite);
        void drawRect(float x, float y, float width, float height, rgba color) override;
        void drawImg(graphics::IDrawable, float x, float y, float scaleX, float scaleY) override;
        bool loadTexture(const std::string& key, const std::string& filepath) override;
        void drawSprite(const std::string& textureKey, float x, float y, float width, float height) override;
        bool loadFont(const std::string& key, const std::string& filepath) override;
        void drawText(const std::string& fontKey, const std::string& text, float x, float y, unsigned int size, rgba color) override;

        void* getNativeHandle() override;

        void clear() override;
        void display() override;

    private:
        sf::RenderWindow _window;
        std::unordered_map<std::string, sf::Texture> _textures;
        std::unordered_map<std::string, sf::Font> _fonts;
};

#endif /* !SFMLWINDOW_HPP_ */
