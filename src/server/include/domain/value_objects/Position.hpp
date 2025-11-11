/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Position
*/

#ifndef POSITION_HPP_
#define POSITION_HPP_

#include "domain/exceptions/PositionException.hpp"

namespace domain::value_objects {
    class Position {
        private:
            float _x;
            float _y;
            float _z;
            void validate(float x, float y, float z);

        public:
            explicit Position(float x = 0.0f, float y = 0.0f, float z = 0.0f);
            float getX() const;
            float getY() const;
            float getZ() const;
            Position move(float dx, float dy, float dz) const;
            bool operator==(const Position& other) const;
            bool operator!=(const Position& other) const;
    };
}

#endif /* !POSITION_HPP_ */
