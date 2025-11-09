/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PositionException
*/

#ifndef POSITIONEXCEPTION_HPP_
#define POSITIONEXCEPTION_HPP_

#include "DomainException.hpp"

namespace domain::exceptions {
    class PositionException: public DomainException {
        public:
            explicit PositionException(float x, float y, float z)
                : DomainException(buildMessage(x, y, z)) {}

        private:
            static std::string buildMessage(float x, float y, float z) {
                std::ostringstream oss;
                oss << "Position out of bounds: x=" << x << ", y=" << y << ", z=" << z
                    << ". Valid range: [-1000.0, 1000.0] for each axis.";
                return oss.str();
            }
    };
}

#endif /* !POSITIONEXCEPTION_HPP_ */
