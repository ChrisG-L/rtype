/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Login
*/

#ifndef LOGIN_HPP_
#define LOGIN_HPP_

#include "domain/value_objects/player/PlayerId.hpp"
#include "application/ports/out/persistence/IUserRepository.hpp"
#include "application/ports/out/ILogger.hpp"
#include "Protocol.hpp"

namespace application::use_cases::auth {
    using domain::value_objects::player::PlayerId;
    using application::ports::out::persistence::IUserRepository;
    using application::ports::out::ILogger;
    using domain::entities::User;

    class Login {
        private:
            std::shared_ptr<IUserRepository> _userRepository;
            std::shared_ptr<ILogger> _logger;

        public:
            explicit Login(
                std::shared_ptr<IUserRepository> userRepository,
                std::shared_ptr<ILogger> logger);
            std::optional<User> execute(const std::string& username, const std::string& password);
    };
}
#endif /* !LOGIN_HPP_ */
