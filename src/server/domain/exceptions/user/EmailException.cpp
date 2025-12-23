/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** EmailException
*/

#include "domain/exceptions/user/EmailException.hpp"
#include <sstream>

namespace domain::exceptions::user {
    EmailException::EmailException(std::string email)
        : DomainException(buildMessage(email)) {}

    std::string EmailException::buildMessage(std::string email) {
        std::ostringstream oss;
        oss << "Invalid email format: " << email << ".";
        return oss.str();
    }
}
