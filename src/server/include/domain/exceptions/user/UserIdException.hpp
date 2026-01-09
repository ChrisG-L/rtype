/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** UserIdException
*/

#ifndef USERIDEXCEPTION_HPP_
#define USERIDEXCEPTION_HPP_

#include "../DomainException.hpp"

namespace domain::exceptions::user {
    class UserIdException: public DomainException {
        public:
            explicit UserIdException(std::string id);

        private:
            static std::string buildMessage(std::string id);
    };
}

#endif /* !USERIDEXCEPTION_HPP_ */
