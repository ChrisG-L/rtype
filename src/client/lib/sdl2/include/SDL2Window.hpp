/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SDL2Window
*/

#ifndef SDL2WINDOW_HPP_
#define SDL2WINDOW_HPP_

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <unordered_map>

#include "graphics/IWindow.hpp"
#include "graphics/IDrawable.hpp"
#include "utils/Vecs.hpp"

class SDL2Window: public graphics::IWindow {
    public:
        SDL2Window(Vec2u winSize, const std::string& name);
        ~SDL2Window();

        Vec2u getSize() const override;
        bool isOpen() override;
        void close() override;
        events::Event pollEvent() override;

        void draw(const graphics::IDrawable& drawable) override;
        void drawRect(float x, float y, float width, float height, rgba color) override;
        void drawImg(graphics::IDrawable, float x, float y, float scaleX, float scaleY) override;
        bool loadTexture(const std::string& key, const std::string& filepath) override;
        void drawSprite(const std::string& textureKey, float x, float y, float width, float height) override;
        bool loadFont(const std::string& key, const std::string& filepath) override;
        void drawText(const std::string& fontKey, const std::string& text, float x, float y, unsigned int size, rgba color) override;

        void* getNativeHandle() override;

        void clear() override;
        void display() override;

        // Post-processing shader support (stubs - not supported in SDL2)
        bool loadShader(const std::string& key, const std::string& vertexPath, const std::string& fragmentPath) override;
        void setPostProcessShader(const std::string& key) override;
        void clearPostProcessShader() override;
        void setShaderUniform(const std::string& name, int value) override;
        bool supportsShaders() const override;

        // Frame management
        void beginFrame() override;
        void endFrame() override;

        SDL_Renderer* getRenderer() const { return _renderer; }

    private:
        SDL_Window* _window;
        SDL_Renderer* _renderer;
        bool _isOpen;
        bool _ttfInitialized = false;
        std::unordered_map<std::string, SDL_Texture*> _textures;
        std::unordered_map<std::string, TTF_Font*> _fonts;
};

#endif /* !SDL2WINDOW_HPP_ */
