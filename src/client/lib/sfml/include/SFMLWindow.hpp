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

        // Post-processing shader support
        bool loadShader(const std::string& key, const std::string& vertexPath, const std::string& fragmentPath) override;
        void setPostProcessShader(const std::string& key) override;
        void clearPostProcessShader() override;
        void setShaderUniform(const std::string& name, int value) override;
        bool supportsShaders() const override;

        // Frame management with post-processing
        void beginFrame() override;
        void endFrame() override;

        // Fullscreen support
        void setFullscreen(bool enabled) override;
        void toggleFullscreen() override;
        bool isFullscreen() const override;

    private:
        bool _isFullscreen = false;
        std::string _windowTitle;
        sf::RenderWindow _window;
        std::unordered_map<std::string, sf::Texture> _textures;
        std::unordered_map<std::string, sf::Font> _fonts;

        // Post-processing pipeline
        sf::RenderTexture _renderTexture;
        std::unordered_map<std::string, sf::Shader> _shaders;
        sf::Shader* _activePostProcessShader = nullptr;
        bool _renderTextureInitialized = false;

        void initRenderTexture();
        sf::RenderTarget& getRenderTarget();
};

#endif /* !SFMLWINDOW_HPP_ */
