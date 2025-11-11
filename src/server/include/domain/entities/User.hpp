/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** User
*/

#ifndef USER_HPP_
#define USER_HPP_

#include "domain/value_objects/user/UserId.hpp"
#include "domain/value_objects/user/Username.hpp"
#include "domain/value_objects/user/Password.hpp"
#include <chrono>

namespace domain::entities {
    class User {
        private:
            value_objects::user::UserId _id;
            value_objects::user::Username _username;
            value_objects::user::Password _passwordHash;
            std::chrono::system_clock::time_point _createdAt;
            std::chrono::system_clock::time_point _lastLogin;

        public:
            explicit User(
                value_objects::user::UserId id,
                value_objects::user::Username username,
                value_objects::user::Password passwordHash,
                std::chrono::system_clock::time_point createdAt = std::chrono::system_clock::now(),
                std::chrono::system_clock::time_point lastLogin = std::chrono::system_clock::now()
            );

            const value_objects::user::UserId& getId() const;
            const value_objects::user::Username& getUsername() const;
            const value_objects::user::Password& getPasswordHash() const;
            const std::chrono::system_clock::time_point& getCreatedAt() const;
            const std::chrono::system_clock::time_point& getLastLogin() const;

            bool verifyPassword(const std::string& password) const;
            void updateLastLogin();
    };
}

#endif /* !USER_HPP_ */
