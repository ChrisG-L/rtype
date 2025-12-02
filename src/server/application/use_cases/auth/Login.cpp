/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Login
*/

#include "application/use_cases/auth/Login.hpp"

namespace application::use_cases::auth {
    Login::Login(std::shared_ptr<IUserRespository> userRepository) : _userRespository(userRepository) {}

    std::optional<User> Login::execute(const std::string& username, const std::string& password) {
        auto playerOpt = _userRespository->findByName(username);
        if (!playerOpt.has_value()) {
            std::cout << "User name not found" << std::endl; 
            return std::nullopt;
        }

        auto user = playerOpt.value();
        std::cout << "password: " << password << std::endl;
        if (!user.verifyPassword(user.getPasswordHash().value(), password)) {
            std::cout << "Wrong password" << std::endl;
            return std::nullopt;
        }
        user.updateLastLogin();
        _userRespository->update(user);
        return playerOpt;
    }
}

