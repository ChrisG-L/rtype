/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Position
*/

#ifndef POSITION_HPP_
#define POSITION_HPP_

#include "exceptions/PositionException.hpp"

namespace domain::value_objects {
    class Position {
        private:
            float _x;
            float _y;
            float _z;

            void validate(float x, float y, float z) {
                if (x < -1000.0f || x > 1000.0f ||
                    y < -1000.0f || y > 1000.0f ||
                    z < -1000.0f || z > 1000.0f) {
                    throw exceptions::PositionException(x, y, z);
                }
            }

        public:
            explicit Position(float x = 0.0f, float y = 0.0f, float z = 0.0f)
                : _x(x), _y(y), _z(z) {
                validate(x, y, z);
            }

            float getX() const { return _x; }
            float getY() const { return _y; }
            float getZ() const { return _z; }

            Position move(float dx, float dy, float dz) const {
                return Position(_x + dx, _y + dy, _z + dz);
            }

            bool operator==(const Position& other) const {
                return _x == other._x && _y == other._y && _z == other._z;
            }

            bool operator!=(const Position& other) const {
                return !(*this == other);
            }
    };
}

#endif /* !POSITION_HPP_ */
