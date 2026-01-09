/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** UserIdException
*/

#include "domain/exceptions/user/UserIdException.hpp"
#include <sstream>

namespace domain::exceptions::user {
    UserIdException::UserIdException(std::string id)
        : DomainException(buildMessage(id)) {}

    std::string UserIdException::buildMessage(std::string id) {
        std::ostringstream oss;
        oss << "User ID cannot be empty or invalid: " << id << ".";
        return oss.str();
    }
}
