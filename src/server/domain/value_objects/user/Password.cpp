/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Password
*/

#include "domain/value_objects/user/Password.hpp"

namespace domain::value_objects::user {
    void Password::validate(const std::string &password) {
        if (password.length() < 6) {
            throw exceptions::user::PasswordException(password);
        }
    }

    std::string Password::hashPassword(std::string password) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.length(), hash);
        std::stringstream ss;
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(hash[i]);
        }
        return ss.str();
    }

    Password::Password(const std::string& password) : _passwordHash(password) {
        validate(password);
    }

    std::string Password::value() const {
        return _passwordHash;
    }

    bool Password::verify(std::string password) {
        if (hashPassword(password) == password)
            return true;
        return false;
    }

    bool Password::operator==(const Password& other) const {
        return _passwordHash == other._passwordHash;
    }

    bool Password::operator!=(const Password& other) const {
        return !(*this == other);
    }
}
