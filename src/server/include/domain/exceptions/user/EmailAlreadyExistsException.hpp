/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** EmailAlreadyExistsException
*/

#ifndef EMAILALREADYEXISTSEXCEPTION_HPP_
#define EMAILALREADYEXISTSEXCEPTION_HPP_

#include "domain/exceptions/DomainException.hpp"
#include <sstream>

namespace domain::exceptions::user {
    class EmailAlreadyExistsException: public DomainException {
        public:
            explicit EmailAlreadyExistsException(std::string email)
                : DomainException(buildMessage(email)) {}

        private:
            static std::string buildMessage(std::string email) {
                std::ostringstream oss;
                oss << "Email already exists: " << email;
                return oss.str();
            }
    };
}

#endif /* !EMAILALREADYEXISTSEXCEPTION_HPP_ */
