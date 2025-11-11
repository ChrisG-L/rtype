/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** UsernameException
*/

#ifndef USERNAMEEXCEPTION_HPP_
#define USERNAMEEXCEPTION_HPP_

#include "../DomainException.hpp"

namespace domain::exceptions::user {
    class UsernameException: public DomainException {
        public:
            explicit UsernameException(std::string username);

        private:
            static std::string buildMessage(std::string username);
    };
}

#endif /* !USERNAMEEXCEPTION_HPP_ */
