/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Position
*/

#ifndef POSITION_HPP_
#define POSITION_HPP_

#include "IComponent.hpp"

class Position: public IComponent {
    public:
        Position(float x, float y, float z): _x(x), _y(y), _z(z) {}
        float _x;
        float _y;
        float _z;
};

#endif /* !POSITION_HPP_ */
