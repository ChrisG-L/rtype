/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Execute
*/

#ifndef EXECUTE_HPP_
#define EXECUTE_HPP_

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "../protocol/Command.hpp"
#include "auth/ExecuteAuth.hpp"
#include "Protocol.hpp"
#include "domain/entities/User.hpp"
#include "application/ports/out/persistence/IUserRepository.hpp"
#include "application/ports/out/IIdGenerator.hpp"
#include "application/ports/out/ILogger.hpp"

namespace infrastructure::adapters::in::network::execute {
    using application::use_cases::auth::Login;
    using application::use_cases::auth::Register;
    using application::ports::out::persistence::IUserRepository;
    using application::ports::out::IIdGenerator;
    using application::ports::out::ILogger;
    using domain::entities::User;

    class Execute {
        public:
            Execute(
                const Command& cmd,
                std::shared_ptr<IUserRepository> userRepository,
                std::shared_ptr<IIdGenerator> idGenerator,
                std::shared_ptr<ILogger> logger,
                std::function<void(const User& user)> onLoginSuccess
            );

        protected:
        private:
            Command _cmd;
            std::unique_ptr<auth::ExecuteAuth> _executeAuth;
    };
}


#endif /* !EXECUTE_HPP_ */
