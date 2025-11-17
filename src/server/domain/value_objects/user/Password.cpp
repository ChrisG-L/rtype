/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Password
*/

#include "domain/value_objects/user/Password.hpp"

namespace domain::value_objects::user {
    void Password::validate(const std::string &password) {
        if (password.length() < 6) {
            throw exceptions::user::PasswordException(password);
        }
    }

    Password::Password(const std::string& password) : _passwordHash(password) {
        validate(password);
    }

    std::string Password::value() const {
        return _passwordHash;
    }

    bool Password::verify(std::string password) {
        if (utils::hashPassword(password) == password)
            return true;
        return false;
    }

    bool Password::operator==(const Password& other) const {
        return _passwordHash == other._passwordHash;
    }

    bool Password::operator!=(const Password& other) const {
        return !(*this == other);
    }
}
