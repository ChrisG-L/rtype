/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** PasswordUtils
*/

#include "domain/value_objects/user/utils/PasswordUtils.hpp"

namespace domain::value_objects::user::utils {
    std::string hashPassword(std::string password) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.length(), hash);
        std::stringstream ss;
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(hash[i]);
        }
        return ss.str();
    }
}

