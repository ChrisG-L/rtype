/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** SFMLWindow
*/

#include "implementations/sfml/SFMLWindow.hpp"
#include "utils/Vecs.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>

bool SFMLWindow::initialize(Vec2u winSize, const std::string& name)
{
    if (_window.isOpen())
        return false;

    _window.create(sf::VideoMode({winSize.x, winSize.y}), name);
    _window.setSize(sf::Vector2u(winSize.x, winSize.y));
    return true;
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

void SFMLWindow::drawImg(sf::Sprite& imgS, float x, float y, float scaleX, float scaleY) {
    imgS.setPosition({x, y});
    imgS.setScale({scaleX, scaleY});
    _window.draw(imgS);
}

void SFMLWindow::clear() {
    _window.clear();
}

void SFMLWindow::display() {
    _window.display();
}
