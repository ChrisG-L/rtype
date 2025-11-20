/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PasswordException
*/

#include "domain/exceptions/user/PasswordException.hpp"
#include <sstream>

namespace domain::exceptions::user {
    PasswordException::PasswordException(std::string password)
        : DomainException(buildMessage(password)) {}

    std::string PasswordException::buildMessage(std::string /*password*/) {
        std::ostringstream oss;
        oss << "The password has to be more than 6 characters.";
        return oss.str();
    }
}
