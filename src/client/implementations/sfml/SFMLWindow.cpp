/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** SFMLWindow
*/

#include "implementations/sfml/SFMLWindow.hpp"

bool SFMLWindow::initialize(Vec2u winSize, const std::string& name)
{
    if (_window.isOpen())
        return false;

    _window.create(sf::VideoMode({winSize.x, winSize.y}), name);
    return true;
}

bool SFMLWindow::isOpen()
{
    return _window.isOpen();
}

std::optional<sf::Event> SFMLWindow::pollEvent()
{
    return _window.pollEvent();
}

void SFMLWindow::draw(const sf::Sprite& sprite) {
    _window.draw(sprite);
}

void SFMLWindow::drawRect(float x, float y, float width, float height, sf::Color color) {
    sf::RectangleShape rect({width, height});
    rect.setPosition({x, y});
    rect.setFillColor(color);
    _window.draw(rect);
}

void SFMLWindow::clear() {
    _window.clear();
}

void SFMLWindow::display() {
    _window.display();
}
