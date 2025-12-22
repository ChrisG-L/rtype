/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SDL2Window
*/

#include "SDL2Window.hpp"
#include "events/Event.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdexcept>

static events::Key scancodeToKey(SDL_Scancode scancode)
{
    switch (scancode) {
        case SDL_SCANCODE_A: return events::Key::A;
        case SDL_SCANCODE_B: return events::Key::B;
        case SDL_SCANCODE_C: return events::Key::C;
        case SDL_SCANCODE_D: return events::Key::D;
        case SDL_SCANCODE_E: return events::Key::E;
        case SDL_SCANCODE_F: return events::Key::F;
        case SDL_SCANCODE_G: return events::Key::G;
        case SDL_SCANCODE_H: return events::Key::H;
        case SDL_SCANCODE_I: return events::Key::I;
        case SDL_SCANCODE_J: return events::Key::J;
        case SDL_SCANCODE_K: return events::Key::K;
        case SDL_SCANCODE_L: return events::Key::L;
        case SDL_SCANCODE_M: return events::Key::M;
        case SDL_SCANCODE_N: return events::Key::N;
        case SDL_SCANCODE_O: return events::Key::O;
        case SDL_SCANCODE_P: return events::Key::P;
        case SDL_SCANCODE_Q: return events::Key::Q;
        case SDL_SCANCODE_R: return events::Key::R;
        case SDL_SCANCODE_S: return events::Key::S;
        case SDL_SCANCODE_T: return events::Key::T;
        case SDL_SCANCODE_U: return events::Key::U;
        case SDL_SCANCODE_V: return events::Key::V;
        case SDL_SCANCODE_W: return events::Key::W;
        case SDL_SCANCODE_X: return events::Key::X;
        case SDL_SCANCODE_Y: return events::Key::Y;
        case SDL_SCANCODE_Z: return events::Key::Z;
        case SDL_SCANCODE_0: return events::Key::Num0;
        case SDL_SCANCODE_1: return events::Key::Num1;
        case SDL_SCANCODE_2: return events::Key::Num2;
        case SDL_SCANCODE_3: return events::Key::Num3;
        case SDL_SCANCODE_4: return events::Key::Num4;
        case SDL_SCANCODE_5: return events::Key::Num5;
        case SDL_SCANCODE_6: return events::Key::Num6;
        case SDL_SCANCODE_7: return events::Key::Num7;
        case SDL_SCANCODE_8: return events::Key::Num8;
        case SDL_SCANCODE_9: return events::Key::Num9;
        case SDL_SCANCODE_SPACE: return events::Key::Space;
        case SDL_SCANCODE_RETURN: return events::Key::Enter;
        case SDL_SCANCODE_ESCAPE: return events::Key::Escape;
        case SDL_SCANCODE_TAB: return events::Key::Tab;
        case SDL_SCANCODE_BACKSPACE: return events::Key::Backspace;
        case SDL_SCANCODE_UP: return events::Key::Up;
        case SDL_SCANCODE_DOWN: return events::Key::Down;
        case SDL_SCANCODE_LEFT: return events::Key::Left;
        case SDL_SCANCODE_RIGHT: return events::Key::Right;
        case SDL_SCANCODE_LSHIFT: return events::Key::LShift;
        case SDL_SCANCODE_RSHIFT: return events::Key::RShift;
        case SDL_SCANCODE_LCTRL: return events::Key::LCtrl;
        case SDL_SCANCODE_RCTRL: return events::Key::RCtrl;
        case SDL_SCANCODE_LALT: return events::Key::LAlt;
        case SDL_SCANCODE_RALT: return events::Key::RAlt;
        default: return events::Key::Unknown;
    }
}

SDL2Window::SDL2Window(Vec2u winSize, const std::string& name)
    : _window(nullptr), _renderer(nullptr), _isOpen(true)
{
    // Windows: désactiver le DPI scaling pour avoir des pixels 1:1
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "0");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
    }

    _window = SDL_CreateWindow(
        name.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        static_cast<int>(winSize.x),
        static_cast<int>(winSize.y),
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!_window) {
        SDL_Quit();
        throw std::runtime_error("Failed to create SDL window: " + std::string(SDL_GetError()));
    }

    _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!_renderer) {
        SDL_DestroyWindow(_window);
        SDL_Quit();
        throw std::runtime_error("Failed to create SDL renderer: " + std::string(SDL_GetError()));
    }
}

SDL2Window::~SDL2Window()
{
    for (auto& [key, texture] : _textures) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
    _textures.clear();

    for (auto& [key, font] : _fonts) {
        if (font) {
            TTF_CloseFont(font);
        }
    }
    _fonts.clear();

    if (_ttfInitialized) {
        TTF_Quit();
    }

    if (_renderer) {
        SDL_DestroyRenderer(_renderer);
    }
    if (_window) {
        SDL_DestroyWindow(_window);
    }
    SDL_Quit();
}

Vec2u SDL2Window::getSize() const
{
    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    return Vec2u(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
}

bool SDL2Window::isOpen()
{
    return _isOpen;
}

void SDL2Window::close()
{
    _isOpen = false;
}

events::Event SDL2Window::pollEvent()
{
    SDL_Event sdlEvent;
    if (SDL_PollEvent(&sdlEvent)) {
        switch (sdlEvent.type) {
            case SDL_QUIT:
                _isOpen = false;
                return events::WindowClosed{};
            case SDL_KEYDOWN:
                if (sdlEvent.key.repeat == 0)
                    return events::KeyPressed{scancodeToKey(sdlEvent.key.keysym.scancode)};
                break;
            case SDL_KEYUP:
                return events::KeyReleased{scancodeToKey(sdlEvent.key.keysym.scancode)};
            default:
                break;
        }
        return pollEvent();
    }
    return events::None{};
}

void* SDL2Window::getNativeHandle()
{
    return _renderer;
}

void SDL2Window::draw(const graphics::IDrawable& drawable)
{
}

void SDL2Window::drawRect(float x, float y, float width, float height, rgba color)
{
    SDL_FRect rect = {x, y, width, height};
    SDL_SetRenderDrawColor(_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRectF(_renderer, &rect);
}

void SDL2Window::drawImg(graphics::IDrawable drawable, float x, float y, float scaleX, float scaleY)
{
}

bool SDL2Window::loadTexture(const std::string& key, const std::string& filepath)
{
    if (_textures.count(key)) {
        return true;
    }

    SDL_Surface* surface = IMG_Load(filepath.c_str());
    if (!surface) {
        return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        return false;
    }

    _textures[key] = texture;
    return true;
}

void SDL2Window::drawSprite(const std::string& textureKey, float x, float y, float width, float height)
{
    auto it = _textures.find(textureKey);
    if (it == _textures.end()) {
        drawRect(x, y, width, height, {255, 0, 255, 255});
        return;
    }

    SDL_FRect destRect = {x, y, width, height};
    SDL_RenderCopyF(_renderer, it->second, nullptr, &destRect);
}

bool SDL2Window::loadFont(const std::string& key, const std::string& filepath)
{
    if (_fonts.count(key)) {
        return true;
    }

    if (!_ttfInitialized) {
        if (TTF_Init() < 0) {
            return false;
        }
        _ttfInitialized = true;
    }

    TTF_Font* font = TTF_OpenFont(filepath.c_str(), 16);
    if (!font) {
        return false;
    }

    _fonts[key] = font;
    return true;
}

void SDL2Window::drawText(const std::string& fontKey, const std::string& text, float x, float y, unsigned int size, rgba color)
{
    auto it = _fonts.find(fontKey);
    if (it == _fonts.end()) {
        return;
    }

    TTF_SetFontSize(it->second, static_cast<int>(size));
    SDL_Color sdlColor = {
        static_cast<Uint8>(color.r),
        static_cast<Uint8>(color.g),
        static_cast<Uint8>(color.b),
        static_cast<Uint8>(color.a)
    };
    SDL_Surface* surface = TTF_RenderText_Blended(it->second, text.c_str(), sdlColor);
    if (!surface) {
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        return;
    }

    int texW, texH;
    SDL_QueryTexture(texture, nullptr, nullptr, &texW, &texH);
    SDL_FRect destRect = {x, y, static_cast<float>(texW), static_cast<float>(texH)};
    SDL_RenderCopyF(_renderer, texture, nullptr, &destRect);
    SDL_DestroyTexture(texture);
}

void SDL2Window::clear()
{
    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
    SDL_RenderClear(_renderer);
}

void SDL2Window::display()
{
    SDL_RenderPresent(_renderer);
}
