/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** ExecuteAuth
*/

#include "infrastructure/adapters/in/network/execute/auth/ExecuteAuth.hpp"

namespace infrastructure::adapters::in::network::execute::auth {
    ExecuteAuth::ExecuteAuth(
        const Command& cmd,
        std::shared_ptr<Login> loginUser,
        std::shared_ptr<Register> registerUser
    ): _cmd(cmd), _login(loginUser), _register(registerUser)
    {
        if (_cmd.type == "LOGIN") {
            login();
        } else if (_cmd.type == "REGISTER") {
            signupUser();
        } else {
            std::cout << "COMMAND NOT FOUND!" << std::endl;
        }
    }

    void ExecuteAuth::login() {
        
    }

    void ExecuteAuth::signupUser() {
        std::string username;
        std::string email;
        std::string unHashedPassword;
        auto it = _cmd.args;
        for (size_t i = 0; i < it.size(); i++) {
            if (i == 1)
                username = it[i];
            else if (i == 2)
                email = it[i];
            else if (i == 3)
                unHashedPassword = it[i];
        }
        _register->execute(username, email, unHashedPassword);
    }
}



