/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Position
*/

#include "domain/value_objects/Position.hpp"

namespace domain::value_objects {
    void Position::validate(float x, float y, float z) {
        if (x < -1000.0f || x > 1000.0f ||
            y < -1000.0f || y > 1000.0f ||
            z < -1000.0f || z > 1000.0f) {
            throw exceptions::PositionException(x, y, z);
        }
    }

    Position::Position(float x, float y, float z) : _x(x), _y(y), _z(z) {
        validate(x, y, z);
    }

    float Position::getX() const {
        return _x;
    }

    float Position::getY() const {
        return _y;
    }

    float Position::getZ() const {
        return _z;
    }

    Position Position::move(float dx, float dy, float dz) const {
        return Position(_x + dx, _y + dy, _z + dz);
    }

    bool Position::operator==(const Position& other) const {
        return _x == other._x && _y == other._y && _z == other._z;
    }

    bool Position::operator!=(const Position& other) const {
        return !(*this == other);
    }
}
