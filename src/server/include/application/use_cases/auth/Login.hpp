/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Login
*/

#ifndef LOGIN_HPP_
#define LOGIN_HPP_

#include "domain/value_objects/player/PlayerId.hpp"
#include "application/ports/out/persistence/IUserRespository.hpp"

#include <iostream>

namespace application::use_cases::auth {
    using domain::value_objects::player::PlayerId;
    using application::ports::out::persistence::IUserRespository;

    class Login {
        private:
            std::shared_ptr<IUserRespository> _userRespository;

        public:
            explicit Login(std::shared_ptr<IUserRespository> userRepository);
            void execute(const std::string& username, const std::string& password);
    };
}
#endif /* !LOGIN_HPP_ */
