/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** HealthException
*/

#include "domain/exceptions/HealthException.hpp"
#include <sstream>

namespace domain::exceptions {
    HealthException::HealthException(float health)
        : DomainException(buildMessage(health)) {}

    std::string HealthException::buildMessage(float health) {
        std::ostringstream oss;
        oss << "Health must be over 0 point and minus 5 points: " << health << ".";
        return oss.str();
    }
}
