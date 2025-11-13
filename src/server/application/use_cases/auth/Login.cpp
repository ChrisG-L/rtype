/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Login
*/

#include "application/use_cases/auth/Login.hpp"

namespace application::use_cases::auth {
    Login::Login(IUserRespository* repo) : repository(repo) {}

    void Login::execute(const std::string& name, const std::string& password) {
        auto playerOpt = repository->findByName(name);
        if (!playerOpt.has_value()) {
            std::cout << "User name not found" << std::endl; 
            return;
        }

        auto user = playerOpt.value();
        std::cout << "password: " << password << std::endl;
        if (!user.verifyPassword(password))
            std::cout << "Wrong password" << std::endl;
            return;
        user.updateLastLogin();
        repository->update(user);
    }
}

