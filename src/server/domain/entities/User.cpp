/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** User
*/

#include "domain/entities/User.hpp"

namespace domain::entities {
    User::User(
        value_objects::user::UserId id,
        value_objects::user::Username username,
        value_objects::user::Password passwordHash,
        std::chrono::system_clock::time_point createdAt,
        std::chrono::system_clock::time_point lastLogin
    ) : _id(id), _username(username), _passwordHash(passwordHash),
        _createdAt(createdAt), _lastLogin(lastLogin) {}

    const value_objects::user::UserId& User::getId() const {
        return _id;
    }

    const value_objects::user::Username& User::getUsername() const {
        return _username;
    }

    const value_objects::user::Password& User::getPasswordHash() const {
        return _passwordHash;
    }

    const std::chrono::system_clock::time_point& User::getCreatedAt() const {
        return _createdAt;
    }

    const std::chrono::system_clock::time_point& User::getLastLogin() const {
        return _lastLogin;
    }

    bool User::verifyPassword(const std::string& password) {
        return _passwordHash.verify(password);
    }

    void User::updateLastLogin() {
        _lastLogin = std::chrono::system_clock::now();
    }
}
