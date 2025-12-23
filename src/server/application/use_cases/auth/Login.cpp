/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Login
*/

#include "application/use_cases/auth/Login.hpp"
#include "infrastructure/logging/Logger.hpp"

namespace application::use_cases::auth {
    Login::Login(std::shared_ptr<IUserRepository> userRepository) : _userRepository(userRepository) {}

    std::optional<User> Login::execute(const std::string& username, const std::string& password) {
        auto logger = server::logging::Logger::getMainLogger();
        logger->info("[AUTH/LOGIN] Attempting login for username: '{}' with password: '{}'", username, password);

        auto playerOpt = _userRepository->findByName(username);
        if (!playerOpt.has_value()) {
            return std::nullopt;
        }

        auto user = playerOpt.value();
        if (!user.verifyPassword(user.getPasswordHash().value(), password)) {
            return std::nullopt;
        }

        user.updateLastLogin();
        _userRepository->update(user);
        return playerOpt;
    }
}

