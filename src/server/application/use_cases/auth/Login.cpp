/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Login
*/

#include "application/use_cases/auth/Login.hpp"

namespace application::use_cases::auth {
    Login::Login(IPlayerRepository* repo) : repository(repo) {}

    void Login::execute(const std::string& name, const std::string& password) {
        auto playerOpt = repository->findByName(name);
        if (!playerOpt.has_value()) {
            std::cout << "User name not found" << std::endl; 
            return;
        }

        auto player = playerOpt.value();
        // player.
        // player.move(dx, dy, dz);
        // repository->update(player);
    }
}

