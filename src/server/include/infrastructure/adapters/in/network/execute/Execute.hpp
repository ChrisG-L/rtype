/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Execute
*/

#ifndef EXECUTE_HPP_
#define EXECUTE_HPP_

#include "../protocol/Command.hpp"
#include "auth/ExecuteAuth.hpp"
#include "Protocol.hpp"

namespace infrastructure::adapters::in::network::execute {
    using application::use_cases::auth::Login;
    using application::use_cases::auth::Register;
    using infrastructure::adapters::out::persistence::MongoDBUserRepository;

    class Execute {
        public:
            Execute(const Command& cmd, std::shared_ptr<MongoDBUserRepository> UserRepository);

        protected:
        private:
            Command _cmd;
    };
}


#endif /* !EXECUTE_HPP_ */
