/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** ExecuteAuth
*/

#ifndef EXECUTEAUTH_HPP_
#define EXECUTEAUTH_HPP_

#include <iostream>
#include <memory>

#include "../../protocol/Command.hpp"
#include "application/use_cases/auth/Login.hpp"
#include "application/use_cases/auth/Register.hpp"
#include "infrastructure/adapters/out/persistence/MongoDBUserRepository.hpp"
#include "Protocol.hpp"



namespace infrastructure::adapters::in::network::execute::auth {
    using infrastructure::adapters::out::persistence::MongoDBUserRepository;
    using application::use_cases::auth::Login;
    using application::use_cases::auth::Register;

    class ExecuteAuth {
        public:
            ExecuteAuth(
                const Command& cmd,
                std::shared_ptr<Login> loginUser,
                std::shared_ptr<Register> registerUser
            );

        private:
            Command _cmd;
            std::shared_ptr<Login> _login;
            std::shared_ptr<Register> _register;

            void login();
            void signupUser();
    };
}


#endif /* !EXECUTEAUTH_HPP_ */
