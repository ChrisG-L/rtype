/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** SFMLWindow
*/

#include "SFMLWindow.hpp"
#include "events/Event.hpp"
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <optional>
#include <stdexcept>

static events::MouseButton sfmlButtonToMouseButton(sf::Mouse::Button button)
{
    switch (button) {
        case sf::Mouse::Button::Left: return events::MouseButton::Left;
        case sf::Mouse::Button::Right: return events::MouseButton::Right;
        case sf::Mouse::Button::Middle: return events::MouseButton::Middle;
        default: return events::MouseButton::Unknown;
    }
}

static events::Key scancodeToKey(sf::Keyboard::Scancode scancode)
{
    switch (scancode) {
        case sf::Keyboard::Scancode::A: return events::Key::A;
        case sf::Keyboard::Scancode::B: return events::Key::B;
        case sf::Keyboard::Scancode::C: return events::Key::C;
        case sf::Keyboard::Scancode::D: return events::Key::D;
        case sf::Keyboard::Scancode::E: return events::Key::E;
        case sf::Keyboard::Scancode::F: return events::Key::F;
        case sf::Keyboard::Scancode::G: return events::Key::G;
        case sf::Keyboard::Scancode::H: return events::Key::H;
        case sf::Keyboard::Scancode::I: return events::Key::I;
        case sf::Keyboard::Scancode::J: return events::Key::J;
        case sf::Keyboard::Scancode::K: return events::Key::K;
        case sf::Keyboard::Scancode::L: return events::Key::L;
        case sf::Keyboard::Scancode::M: return events::Key::M;
        case sf::Keyboard::Scancode::N: return events::Key::N;
        case sf::Keyboard::Scancode::O: return events::Key::O;
        case sf::Keyboard::Scancode::P: return events::Key::P;
        case sf::Keyboard::Scancode::Q: return events::Key::Q;
        case sf::Keyboard::Scancode::R: return events::Key::R;
        case sf::Keyboard::Scancode::S: return events::Key::S;
        case sf::Keyboard::Scancode::T: return events::Key::T;
        case sf::Keyboard::Scancode::U: return events::Key::U;
        case sf::Keyboard::Scancode::V: return events::Key::V;
        case sf::Keyboard::Scancode::W: return events::Key::W;
        case sf::Keyboard::Scancode::X: return events::Key::X;
        case sf::Keyboard::Scancode::Y: return events::Key::Y;
        case sf::Keyboard::Scancode::Z: return events::Key::Z;
        case sf::Keyboard::Scancode::Num0: return events::Key::Num0;
        case sf::Keyboard::Scancode::Num1: return events::Key::Num1;
        case sf::Keyboard::Scancode::Num2: return events::Key::Num2;
        case sf::Keyboard::Scancode::Num3: return events::Key::Num3;
        case sf::Keyboard::Scancode::Num4: return events::Key::Num4;
        case sf::Keyboard::Scancode::Num5: return events::Key::Num5;
        case sf::Keyboard::Scancode::Num6: return events::Key::Num6;
        case sf::Keyboard::Scancode::Num7: return events::Key::Num7;
        case sf::Keyboard::Scancode::Num8: return events::Key::Num8;
        case sf::Keyboard::Scancode::Num9: return events::Key::Num9;
        case sf::Keyboard::Scancode::Space: return events::Key::Space;
        case sf::Keyboard::Scancode::Enter: return events::Key::Enter;
        case sf::Keyboard::Scancode::Escape: return events::Key::Escape;
        case sf::Keyboard::Scancode::Tab: return events::Key::Tab;
        case sf::Keyboard::Scancode::Backspace: return events::Key::Backspace;
        case sf::Keyboard::Scancode::Up: return events::Key::Up;
        case sf::Keyboard::Scancode::Down: return events::Key::Down;
        case sf::Keyboard::Scancode::Left: return events::Key::Left;
        case sf::Keyboard::Scancode::Right: return events::Key::Right;
        case sf::Keyboard::Scancode::LShift: return events::Key::LShift;
        case sf::Keyboard::Scancode::RShift: return events::Key::RShift;
        case sf::Keyboard::Scancode::LControl: return events::Key::LCtrl;
        case sf::Keyboard::Scancode::RControl: return events::Key::RCtrl;
        case sf::Keyboard::Scancode::LAlt: return events::Key::LAlt;
        case sf::Keyboard::Scancode::RAlt: return events::Key::RAlt;
        default: return events::Key::Unknown;
    }
}

SFMLWindow::SFMLWindow(Vec2u winSize, const std::string& name) {
    _window.create(sf::VideoMode({winSize.x, winSize.y}), name);
}

Vec2u SFMLWindow::getSize() const {
    return Vec2u(_window.getSize().x, _window.getSize().y);
}


bool SFMLWindow::isOpen()
{
    return _window.isOpen();
}

void SFMLWindow::close()
{
    return _window.close();
}

events::Event SFMLWindow::pollEvent()
{
    if (auto ev = _window.pollEvent()) {
        if (ev->is<sf::Event::Closed>()) {
            _window.close();
            return events::WindowClosed{};
        }
        if (const auto* keyPressed = ev->getIf<sf::Event::KeyPressed>()) {
            return events::KeyPressed{scancodeToKey(keyPressed->scancode)};
        }
        if (const auto* keyReleased = ev->getIf<sf::Event::KeyReleased>()) {
            return events::KeyReleased{scancodeToKey(keyReleased->scancode)};
        }
        if (const auto* mousePressed = ev->getIf<sf::Event::MouseButtonPressed>()) {
            return events::MouseButtonPressed{
                sfmlButtonToMouseButton(mousePressed->button),
                static_cast<int>(mousePressed->position.x),
                static_cast<int>(mousePressed->position.y)
            };
        }
        if (const auto* mouseReleased = ev->getIf<sf::Event::MouseButtonReleased>()) {
            return events::MouseButtonReleased{
                sfmlButtonToMouseButton(mouseReleased->button),
                static_cast<int>(mouseReleased->position.x),
                static_cast<int>(mouseReleased->position.y)
            };
        }
        if (const auto* mouseMoved = ev->getIf<sf::Event::MouseMoved>()) {
            return events::MouseMoved{
                static_cast<int>(mouseMoved->position.x),
                static_cast<int>(mouseMoved->position.y)
            };
        }
        if (const auto* textEntered = ev->getIf<sf::Event::TextEntered>()) {
            return events::TextEntered{textEntered->unicode};
        }
        // Événement non géré (Resized, FocusGained, etc.) → continuer à vider la queue
        return pollEvent();
    }
    return events::None{};
}

void* SFMLWindow::getNativeHandle() {
    return &_window;
}

void SFMLWindow::draw(const graphics::IDrawable& drawable) {
    // _window.drawAll();
}

void SFMLWindow::drawRect(float x, float y, float width, float height, rgba color) {
    sf::RectangleShape rect({width, height});
    rect.setPosition({x, y});
    rect.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
    _window.draw(rect);
}

void SFMLWindow::drawImg(graphics::IDrawable drawable, float x, float y, float scaleX, float scaleY) {
    // imgS.setPosition({x, y});
    // imgS.setScale({scaleX, scaleY});
    // _window.draw(imgS);
}

bool SFMLWindow::loadTexture(const std::string& key, const std::string& filepath) {
    if (_textures.count(key)) {
        return true;
    }

    sf::Texture texture;
    if (!texture.loadFromFile(filepath)) {
        return false;
    }

    _textures[key] = std::move(texture);
    return true;
}

void SFMLWindow::drawSprite(const std::string& textureKey, float x, float y, float width, float height) {
    auto it = _textures.find(textureKey);
    if (it == _textures.end()) {
        drawRect(x, y, width, height, {255, 0, 255, 255});
        return;
    }

    sf::Sprite sprite(it->second);
    sprite.setPosition({x, y});

    sf::Vector2u texSize = it->second.getSize();
    if (texSize.x > 0 && texSize.y > 0) {
        float scaleX = width / static_cast<float>(texSize.x);
        float scaleY = height / static_cast<float>(texSize.y);
        sprite.setScale({scaleX, scaleY});
    }

    _window.draw(sprite);
}

bool SFMLWindow::loadFont(const std::string& key, const std::string& filepath) {
    if (_fonts.count(key)) {
        return true;
    }

    sf::Font font;
    if (!font.openFromFile(filepath)) {
        return false;
    }

    _fonts[key] = std::move(font);
    return true;
}

void SFMLWindow::drawText(const std::string& fontKey, const std::string& text, float x, float y, unsigned int size, rgba color) {
    auto it = _fonts.find(fontKey);
    if (it == _fonts.end()) {
        return;
    }

    sf::Text sfText(it->second, text, size);
    sfText.setPosition({x, y});
    sfText.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
    _window.draw(sfText);
}

void SFMLWindow::clear() {
    _window.clear();
}

void SFMLWindow::display() {
    _window.display();
}
