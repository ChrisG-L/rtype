/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerIdException
*/

#include "domain/exceptions/player/PlayerIdException.hpp"
#include <sstream>

namespace domain::exceptions::player {
    PlayerIdException::PlayerIdException(std::string id)
        : DomainException(buildMessage(id)) {}

    std::string PlayerIdException::buildMessage(std::string id) {
        std::ostringstream oss;
        oss << "Player id must be conform to UUID v7 standards: " << id << ".";
        return oss.str();
    }
}
