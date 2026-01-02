/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Execute
*/

#include "infrastructure/adapters/in/network/execute/Execute.hpp"
#include "domain/entities/User.hpp"
#include "infrastructure/adapters/in/network/execute/auth/ExecuteAuth.hpp"
#include <cstdint>
#include <memory>

namespace infrastructure::adapters::in::network::execute {

    Execute::Execute(
        const Command& cmd,
        std::shared_ptr<IUserRepository> userRepository,
        std::shared_ptr<IIdGenerator> idGenerator,
        std::shared_ptr<ILogger> logger,
        std::function<void(const User& user)> onLoginSuccess
    )
    {
        std::shared_ptr<Login> login = std::make_shared<Login>(userRepository, logger);
        std::shared_ptr<Register> registerUser = std::make_shared<Register>(userRepository, idGenerator, logger);
        _executeAuth = std::make_unique<auth::ExecuteAuth>(cmd, login, registerUser);
        std::optional<User> userOpt = _executeAuth->getUser();
        if (userOpt.has_value()) {
            // Note: "User already logged" check is now handled by SessionManager
            // in TCPAuthServer::handle_command after this callback
            onLoginSuccess(userOpt.value());
        }
    }

}
