/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** User
*/

#include "domain/entities/User.hpp"

namespace domain::entities {
    User::User(
        UserId id,
        Username username,
        Email email,
        Password passwordHash,
        std::chrono::system_clock::time_point lastLogin,
        std::chrono::system_clock::time_point createdAt
    ) : _id(id), _username(username), _email(email), _passwordHash(passwordHash),
        _lastLogin(lastLogin), _createdAt(createdAt) {}

    const UserId& User::getId() const {
        return _id;
    }

    const Username& User::getUsername() const {
        return _username;
    }

    const Email& User::getEmail() const {
        return _email;
    }

    const Password& User::getPasswordHash() const {
        return _passwordHash;
    }

    const std::chrono::system_clock::time_point& User::getCreatedAt() const {
        return _createdAt;
    }

    const std::chrono::system_clock::time_point& User::getLastLogin() const {
        return _lastLogin;
    }

    bool User::verifyPassword(const std::string& hashPassword, const std::string& password) {
        return _passwordHash.verify(hashPassword, password);
    }

    void User::updateLastLogin() {
        _lastLogin = std::chrono::system_clock::now();
    }
}
