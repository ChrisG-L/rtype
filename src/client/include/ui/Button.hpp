/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** Button
*/

#ifndef BUTTON_HPP_
#define BUTTON_HPP_

#include "IUIElement.hpp"
#include "utils/Vecs.hpp"
#include "events/Signal.hpp"
#include <iostream>

namespace ui {
    class Button: public IUIElement {
    public:
        Button(const Vec2f& pos);
        ~Button();

        Signal<std::string, int> onClick;

        Vec2f getPos() const override;
        void setPos(const Vec2f& pos) override;

        void click();
    private:
        Vec2f _pos;
    };
};


#endif /* !BUTTON_HPP_ */
