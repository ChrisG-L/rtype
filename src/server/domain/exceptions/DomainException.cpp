/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** DomainException
*/

#include "domain/exceptions/DomainException.hpp"

namespace domain::exceptions {
    DomainException::DomainException(const std::string& message) : _message(message) {}

    const char* DomainException::what() const noexcept {
        return _message.c_str();
    }
}
