/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** EmailException
*/

#ifndef EMAILEXCEPTION_HPP_
#define EMAILEXCEPTION_HPP_

#include "../DomainException.hpp"

namespace domain::exceptions::user {
    class EmailException: public DomainException {
        public:
            explicit EmailException(std::string email);

        private:
            static std::string buildMessage(std::string email);
    };
}

#endif /* !EMAILEXCEPTION_HPP_ */
