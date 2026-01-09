/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** ExecuteAuth
*/

#include "infrastructure/adapters/in/network/execute/auth/ExecuteAuth.hpp"
#include "infrastructure/logging/Logger.hpp"
#include "Protocol.hpp"
#include "application/use_cases/auth/Login.hpp"

namespace infrastructure::adapters::in::network::execute::auth {
    ExecuteAuth::ExecuteAuth(
        const Command& cmd,
        std::shared_ptr<Login> loginUser,
        std::shared_ptr<Register> registerUser
    ): _cmd(cmd), _login(loginUser), _register(registerUser), _user{}
    {
        // Client sends Login/Register, server responds with LoginAck/RegisterAck
        if (_cmd.type == static_cast<uint16_t>(MessageType::Login)) {
            login();
        } else if (_cmd.type == static_cast<uint16_t>(MessageType::Register)) {
            signupUser();
        } else {
            server::logging::Logger::getNetworkLogger()->warn("Unknown command type: {}", _cmd.type);
        }
    }

    std::optional<User> ExecuteAuth::getUser() const {
        return _user;
    }

    void ExecuteAuth::login() {
        auto loginOpt = LoginMessage::from_bytes(_cmd.buf.data(), _cmd.buf.size());
        if (!loginOpt) {
            server::logging::Logger::getNetworkLogger()->warn("Invalid LoginMessage received!");
            return;
        }
        _user = _login->execute(loginOpt->username, loginOpt->password);
    }

    void ExecuteAuth::signupUser() {
        auto registerUserOpt = RegisterMessage::from_bytes(_cmd.buf.data(), _cmd.buf.size());
        if (!registerUserOpt) {
            server::logging::Logger::getNetworkLogger()->warn("Invalid RegisterMessage received!");
            return;
        }
        _user = _register->execute(registerUserOpt->username, registerUserOpt->email, registerUserOpt->password);
    }
}



