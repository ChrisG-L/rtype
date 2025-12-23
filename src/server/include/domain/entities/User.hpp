/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** User
*/

#ifndef USER_HPP_
#define USER_HPP_

#include <bsoncxx/oid.hpp>

#include "domain/value_objects/user/UserId.hpp"
#include "domain/value_objects/user/Username.hpp"
#include "domain/value_objects/user/Email.hpp"
#include "domain/value_objects/user/Password.hpp"
#include <chrono>

namespace domain::entities {
    using value_objects::user::UserId;
    using value_objects::user::Username;
    using value_objects::user::Email;
    using value_objects::user::Password;

    class User {
        private:
            UserId _id;
            Username _username;
            Email _email;
            Password _passwordHash;
            std::chrono::system_clock::time_point _lastLogin;
            std::chrono::system_clock::time_point _createdAt;

        public:
            explicit User(
                UserId id,
                Username username,
                Email email,
                Password passwordHash,
                std::chrono::system_clock::time_point lastLogin = std::chrono::system_clock::now(),
                std::chrono::system_clock::time_point createdAt = std::chrono::system_clock::now()
            );

            const UserId& getId() const;
            const Username& getUsername() const;
            const Email& getEmail() const;
            const Password& getPasswordHash() const;
            const std::chrono::system_clock::time_point& getLastLogin() const;
            const std::chrono::system_clock::time_point& getCreatedAt() const;

            bool verifyPassword(const std::string& hashPassword, const std::string& password);
            void updateLastLogin();
    };
}

#endif /* !USER_HPP_ */
