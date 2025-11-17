/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** PasswordUtils
*/

#ifndef PASSWORDUTILS_HPP_
#define PASSWORDUTILS_HPP_

#include <sstream>
#include <string>
#include <openssl/sha.h>
#include <iomanip>

namespace domain::value_objects::user::utils {
    std::string hashPassword(std::string password);
}

#endif /* !PASSWORDUTILS_HPP_ */
