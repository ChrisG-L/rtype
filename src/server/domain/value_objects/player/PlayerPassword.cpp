/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerPassword
*/

#include "domain/value_objects/player/PlayerPassword.hpp"

namespace domain::value_objects::player {
    void PlayerPassword::validate(const std::string &password) {
        if (password.length() < 6)
            throw exceptions::player::PlayerPasswordException(password);
    }

    PlayerPassword::PlayerPassword(const std::string& password) : _password(password) {
        validate(password);
    }

    std::string PlayerPassword::value() const {
        return _password;
    }

    bool PlayerPassword::operator==(const PlayerPassword& other) {
        return _password == other._password;
    }

    bool PlayerPassword::operator!=(const PlayerPassword& other) {
        return !(*this == other);
    }
}

