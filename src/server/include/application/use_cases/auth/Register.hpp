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

#include <optional>

namespace application::use_cases::auth {
    using domain::entities::User;
    using domain::value_objects::player::PlayerId;
    using application::ports::out::persistence::IUserRepository;

    class Register {
        private:
            std::shared_ptr<IUserRepository> _userRepository;

        public:
            explicit Register(std::shared_ptr<IUserRepository> userRepository);
            std::optional<User> execute(
                const std::string& username,
                const std::string& email, 
                const std::string& unHashedPassword);
    };
}

#endif /* !REGISTER_HPP_ */
