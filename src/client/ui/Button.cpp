/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Button
*/

#include "ui/Button.hpp"
#include "utils/Vecs.hpp"

namespace ui {
    Button::Button(const Vec2f& pos): _pos(pos)
    {
    }   

    Button::~Button()
    {
    }

    Vec2f Button::getPos() const {
        return _pos;
    }

    void Button::setPos(const Vec2f& pos) {
        _pos = pos;
    }

    void Button::click() {
        std::cout << "Button onclick" << std::endl;
        onClick.emit("Main button", 32);
    }
}

