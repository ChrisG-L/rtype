/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Execute
*/

#include "infrastructure/adapters/in/network/execute/Execute.hpp"

namespace infrastructure::adapters::in::network::execute {

    Execute::Execute(const Command& cmd, std::shared_ptr<MongoDBUserRepository> UserRepository)
    {
        std::shared_ptr<Login> login = std::make_shared<Login>(UserRepository);
        std::shared_ptr<Register> registerUser = std::make_shared<Register>(UserRepository);
    
        auth::ExecuteAuth executeAuth(cmd, login, registerUser);
    }
}
