/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** ExecuteAuth
*/

#include "infrastructure/adapters/in/network/execute/auth/ExecuteAuth.hpp"
#include "Protocol.hpp"
#include "application/use_cases/auth/Login.hpp"

namespace infrastructure::adapters::in::network::execute::auth {
    ExecuteAuth::ExecuteAuth(
        const Command& cmd,
        std::shared_ptr<Login> loginUser,
        std::shared_ptr<Register> registerUser
    ): _cmd(cmd), _login(loginUser), _register(registerUser), _user{}
    {
        if (_cmd.type == static_cast<uint16_t>(MessageType::LoginAck)) {
            login();
        } else if (_cmd.type == static_cast<uint16_t>(MessageType::RegisterAck)) {
            signupUser();
        } else {
            std::cout << "COMMAND NOT FOUND!" << std::endl;
        }
    }

    std::optional<User> ExecuteAuth::getUser() const {
        return _user;
    }

    void ExecuteAuth::login() {
        LoginMessage login = LoginMessage::from_bytes(_cmd.buf.data());
        _user = _login->execute(login.username, login.password);
    }

    void ExecuteAuth::signupUser() {
        RegisterMessage registerUser = RegisterMessage::from_bytes(_cmd.buf.data());
        _user = _register->execute(registerUser.username, registerUser.email, registerUser.password);
    }
}



