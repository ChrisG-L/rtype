/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** UserAlreadyLoggedException
*/

#ifndef USERALREADYLOGGEDEXCEPTION_HPP_
#define USERALREADYLOGGEDEXCEPTION_HPP_

#include "domain/exceptions/DomainException.hpp"
#include <sstream>

namespace domain::exceptions::user {
    class UserAlreadyLoggedException: public DomainException {
        public:
            explicit UserAlreadyLoggedException(std::string username)
                : DomainException(buildMessage(username)) {}

        private:
            static std::string buildMessage(std::string username) {
                std::ostringstream oss;
                oss << "User '" << username << "' is already logged in";
                return oss.str();
            }
    };
}

#endif /* !USERALREADYLOGGEDEXCEPTION_HPP_ */
