/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Login
*/

#ifndef LOGIN_HPP_
#define LOGIN_HPP_

#include "domain/value_objects/player/PlayerId.hpp"
#include "application/ports/out/persistence/IPlayerRepository.hpp"

#include <iostream>

namespace application::use_cases::auth {
    using domain::value_objects::player::PlayerId;
    using application::ports::out::persistence::IPlayerRepository;

    class Login {
        private:
            IPlayerRepository* repository;

        public:
            explicit Login(IPlayerRepository* repo);
            void execute(const std::string& name, const std::string& password);
    };
}
#endif /* !LOGIN_HPP_ */
