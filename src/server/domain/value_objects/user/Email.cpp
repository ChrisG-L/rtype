/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Email
*/

#include "domain/value_objects/user/Email.hpp"
#include <regex>

namespace domain::value_objects::user {
    void Email::validate(const std::string &email) {
        const std::regex emailPattern(
            R"(^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$)"
        );

        if (!std::regex_match(email, emailPattern)) {
            throw exceptions::user::EmailException(email);
        }

        if (email.length() > 254) {
            throw exceptions::user::EmailException(email);
        }
    }

    Email::Email(const std::string& email) : _email(email) {
        validate(email);
    }

    std::string Email::value() const {
        return _email;
    }

    bool Email::operator==(const Email& other) const {
        return _email == other._email;
    }

    bool Email::operator!=(const Email& other) const {
        return !(*this == other);
    }
}
