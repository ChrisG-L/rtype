/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SDL2Window
*/

#ifndef SDL2WINDOW_HPP_
#define SDL2WINDOW_HPP_

#include <SDL2/SDL.h>
#include <string>

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

        void* getNativeHandle() override;

        void clear() override;
        void display() override;

        SDL_Renderer* getRenderer() const { return _renderer; }

    private:
        SDL_Window* _window;
        SDL_Renderer* _renderer;
        bool _isOpen;
};

#endif /* !SDL2WINDOW_HPP_ */
