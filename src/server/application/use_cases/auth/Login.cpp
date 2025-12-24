/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Login
*/

#include "application/use_cases/auth/Login.hpp"

namespace application::use_cases::auth {
    Login::Login(
        std::shared_ptr<IUserRepository> userRepository,
        std::shared_ptr<ILogger> logger
    ) : _userRepository(userRepository), _logger(logger) {}

    std::optional<User> Login::execute(const std::string& username, const std::string& password) {
        _logger->info("[AUTH/LOGIN] Attempting login for username: '{}'", username);

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
        return user;
    }
}

