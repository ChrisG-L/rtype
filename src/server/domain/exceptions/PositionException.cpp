/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PositionException
*/

#include "domain/exceptions/PositionException.hpp"
#include <sstream>

namespace domain::exceptions {
    PositionException::PositionException(float x, float y, float z)
        : DomainException(buildMessage(x, y, z)) {}

    std::string PositionException::buildMessage(float x, float y, float z) {
        std::ostringstream oss;
        oss << "Position out of bounds: x=" << x << ", y=" << y << ", z=" << z
            << ". Valid range: [-1000.0, 1000.0] for each axis.";
        return oss.str();
    }
}
