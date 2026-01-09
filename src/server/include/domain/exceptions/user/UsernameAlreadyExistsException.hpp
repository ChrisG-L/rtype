/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** UsernameAlreadyExistsException
*/

#ifndef USERNAMEALREADYEXISTSEXCEPTION_HPP_
#define USERNAMEALREADYEXISTSEXCEPTION_HPP_

#include "domain/exceptions/DomainException.hpp"
#include <sstream>

namespace domain::exceptions::user {
    class UsernameAlreadyExistsException: public DomainException {
        public:
            explicit UsernameAlreadyExistsException(std::string username)
                : DomainException(buildMessage(username)) {}

        private:
            static std::string buildMessage(std::string username) {
                std::ostringstream oss;
                oss << "Username already exists: " << username;
                return oss.str();
            }
    };
}

#endif /* !USERNAMEALREADYEXISTSEXCEPTION_HPP_ */
