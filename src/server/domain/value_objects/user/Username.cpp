/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Username
*/

#include "domain/value_objects/user/Username.hpp"

namespace domain::value_objects::user {
    void Username::validate(const std::string &username) {
        if (username.length() < 6 || username.length() > 21) {
            throw exceptions::user::UsernameException(username);
        }
    }

    Username::Username(const std::string& username) : _username(username) {
        validate(username);
    }

    std::string Username::value() const {
        return _username;
    }

    bool Username::operator==(const Username& other) const {
        return _username == other._username;
    }

    bool Username::operator!=(const Username& other) const {
        return !(*this == other);
    }
}
