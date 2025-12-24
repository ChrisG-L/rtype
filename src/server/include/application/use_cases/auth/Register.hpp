/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Register
*/

#ifndef REGISTER_HPP_
#define REGISTER_HPP_

#include "domain/entities/User.hpp"
#include "domain/value_objects/player/PlayerId.hpp"
#include "application/ports/out/persistence/IUserRepository.hpp"
#include "application/ports/out/IIdGenerator.hpp"
#include "application/ports/out/ILogger.hpp"

#include <optional>

namespace application::use_cases::auth {
    using domain::entities::User;
    using domain::value_objects::player::PlayerId;
    using application::ports::out::persistence::IUserRepository;
    using application::ports::out::IIdGenerator;
    using application::ports::out::ILogger;

    class Register {
        private:
            std::shared_ptr<IUserRepository> _userRepository;
            std::shared_ptr<IIdGenerator> _idGenerator;
            std::shared_ptr<ILogger> _logger;

        public:
            explicit Register(
                std::shared_ptr<IUserRepository> userRepository,
                std::shared_ptr<IIdGenerator> idGenerator,
                std::shared_ptr<ILogger> logger);
            std::optional<User> execute(
                const std::string& username,
                const std::string& email,
                const std::string& unHashedPassword);
    };
}

#endif /* !REGISTER_HPP_ */
