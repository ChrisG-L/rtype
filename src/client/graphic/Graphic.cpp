/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Graphic
*/

#include "graphic/Graphic.hpp"

Graphic::Graphic(): _window{sf::VideoMode({1200, 1200}), "R-type window"}
{
    _window.setPosition({700, 250});
}

void Graphic::run()
{
    _window.setActive(false);
    std::thread thread(&Graphic::renderingThread, this, &_window);
    while (_window.isOpen())
    {
        while (const std::optional event = _window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                _window.close();
            
            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                std::cout << "button left pressed" << std::endl;
            }
        }
    }
    thread.join();
}

void Graphic::renderingThread(sf::RenderWindow* window)
{
    const sf::Texture tBedroom("assets/spaceship/bedroom.jpg");
    window->setActive(true);

    sf::Sprite sBedroom(tBedroom);
    sf::Vector2u windowSize = window->getSize();
    sf::Vector2u sTextureSize = tBedroom.getSize();
    
    sBedroom.setScale({
        static_cast<float>(windowSize.x) / sTextureSize.x,
        static_cast<float>(windowSize.y) / sTextureSize.y
    });

    sf::ConvexShape convex;

    convex.setPointCount(4);
    convex.setPoint(0, {0.f, 0.f});
    convex.setPoint(1, {300.f, -30.f});
    convex.setPoint(2, {300.f, 220.f});
    convex.setPoint(3, {0.f, 203.f});
    convex.setPosition({windowSize.x - 470, 500});
    convex.setFillColor({255, 255, 255, 128});

    sf::FloatRect boundingBox = convex.getGlobalBounds();
    const auto hand_cursor = sf::Cursor::createFromSystem(sf::Cursor::Type::Hand).value();
    const auto arrow_cursor = sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow).value();

    while (window->isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(*window);
        window->display();
        window->clear();
        window->draw(sBedroom);
        if (boundingBox.contains({mousePos.x, mousePos.y})) { 
            window->setMouseCursor(hand_cursor);
            window->draw(convex);
        } else {
            window->setMouseCursor(arrow_cursor);
        }

        window->display();
    }
}
