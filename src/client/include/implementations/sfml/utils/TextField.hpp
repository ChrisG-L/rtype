/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** TextField
*/

#ifndef TEXTFIELD_HPP_
#define TEXTFIELD_HPP_

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp>

class TextField: public sf::Transformable, sf::Drawable {
     public:
        TextField(unsigned int maxChars) :
            m_size(maxChars),
            m_rect(sf::Vector2f(15 * m_size, 20)), // 15 pixels per char, 20 pixels height, you can tweak
            m_hasfocus(false)
        {
            m_font.openFromFile("assets/fonts/arial.ttf");
            m_rect.setOutlineThickness(2);
            m_rect.setFillColor(sf::Color::White);
            m_rect.setOutlineColor(sf::Color(127,127,127));
            m_rect.setPosition(this->getPosition());
        }

    private:
        unsigned int m_size;
        sf::Font m_font;
        std::string m_text;
        sf::RectangleShape m_rect;
        bool m_hasfocus;
};

#endif /* !TEXTFIELD_HPP_ */
