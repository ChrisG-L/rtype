/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** SFMLWindow
*/

#include "SFMLWindow.hpp"
#include <SFML/Window/Event.hpp>
#include <optional>
#include <stdexcept>

SFMLWindow::SFMLWindow(Vec2u winSize, const std::string& name): _window{} {
    if (_window.isOpen())
        throw std::runtime_error("Window not opened!");

    _window.create(sf::VideoMode({winSize.x, winSize.y}), name);
    _window.setSize(sf::Vector2u(winSize.x, winSize.y));
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

bool SFMLWindow::pollEvent()
{
    std::optional<sf::Event> pEvent;
    if (auto ev = _window.pollEvent()) {
        pEvent = std::move(ev);
        return false;
    }
    // Convert SFML event to your Event type if needed
    return true;
}

void SFMLWindow::draw(const graphics::IDrawable& drawable) {
    // _window.draw(sprite);
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

void SFMLWindow::clear() {
    _window.clear();
}

void SFMLWindow::display() {
    _window.display();
}
