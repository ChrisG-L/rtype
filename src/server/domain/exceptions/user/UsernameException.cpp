/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** UsernameException
*/

#include "domain/exceptions/user/UsernameException.hpp"
#include <sstream>

namespace domain::exceptions::user {
    UsernameException::UsernameException(std::string username)
        : DomainException(buildMessage(username)) {}

    std::string UsernameException::buildMessage(std::string username) {
        std::ostringstream oss;
        oss << "The username has to be between 3 and 21 characters: " << username << ".";
        return oss.str();
    }
}
