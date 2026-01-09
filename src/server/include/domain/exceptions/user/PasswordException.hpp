/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PasswordException
*/

#ifndef PASSWORDEXCEPTION_HPP_
#define PASSWORDEXCEPTION_HPP_

#include "../DomainException.hpp"

namespace domain::exceptions::user {
    class PasswordException: public DomainException {
        public:
            explicit PasswordException(std::string password);

        private:
            static std::string buildMessage(std::string password);
    };
}

#endif /* !PASSWORDEXCEPTION_HPP_ */
