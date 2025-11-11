/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** UserId
*/

#include "domain/value_objects/user/UserId.hpp"

namespace domain::value_objects::user {
    void UserId::validate(const std::string &id) {
        if (id.empty()) {
            throw exceptions::user::UserIdException(id);
        }
    }

    UserId::UserId(const std::string& id) : _id(id) {
        validate(id);
    }

    std::string UserId::value() const {
        return _id;
    }

    bool UserId::operator==(const UserId& other) const {
        return _id == other._id;
    }

    bool UserId::operator!=(const UserId& other) const {
        return !(*this == other);
    }
}
