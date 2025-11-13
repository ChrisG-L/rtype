/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Execute
*/

#ifndef EXECUTE_HPP_
#define EXECUTE_HPP_

#include <iostream>

#include "../protocol/Command.hpp"
#include "application/use_cases/auth/Login.hpp"

namespace infrastructure::adapters::in::network::execute {
    using application::use_cases::auth::Login;

    class Execute {
        public:
            Execute(const Command& cmd);

        protected:
        private:
            Command _cmd;
    };
}

#endif /* !EXECUTE_HPP_ */
