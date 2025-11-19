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

void SFMLWindow::draw(const sf::Sprite& sprite) {
    _window.draw(sprite);
    // _window.draw(sf::Drawable(drawable));
}

void SFMLWindow::clear() {
    _window.clear();
}

void SFMLWindow::display() {
    _window.display();
}
