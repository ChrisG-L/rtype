/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** IUIElement
*/

#ifndef IUIELEMENT_HPP_
#define IUIELEMENT_HPP_

#include "../utils/Vecs.hpp"

class IUIElement {
    public:
        virtual ~IUIElement() = default;

        virtual Vec2f getPos() const = 0;
        virtual void setPos(const Vec2f& pos) = 0;

    private:
};

#endif /* !IUIELEMENT_HPP_ */
