/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerName
*/

#include "domain/value_objects/player/PlayerName.hpp"

namespace domain::value_objects::player {
    void PlayerName::validate(const std::string &name) {
        if (name.length() < 6 || name.length() > 21)
            throw exceptions::player::PlayerNameException(name);
    }

    PlayerName::PlayerName(const std::string& name) : _name(name) {
        validate(name);
    }

    std::string PlayerName::value() const {
        return _name;
    }

    bool PlayerName::operator==(const PlayerName& other) {
        return _name == other._name;
    }

    bool PlayerName::operator!=(const PlayerName& other) {
        return !(*this == other);
    }
}
