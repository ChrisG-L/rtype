/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerId
*/

#include "domain/value_objects/player/PlayerId.hpp"

namespace domain::value_objects::player {
    void PlayerId::validate(const std::string &id) {
        if (id.empty()) {
            throw exceptions::player::PlayerIdException(id);
        }
    }

    PlayerId::PlayerId(const std::string& id) : _id(id) {
        validate(id);
    }

    std::string PlayerId::value() const {
        return _id;
    }

    bool PlayerId::operator==(const PlayerId& other) {
        return _id == other._id;
    }

    bool PlayerId::operator!=(const PlayerId& other) {
        return !(*this == other);
    }
}
