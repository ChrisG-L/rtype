/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** HealthException
*/

#ifndef HEALTHEXCEPTION_HPP_
#define HEALTHEXCEPTION_HPP_

#include "DomainException.hpp"

namespace domain::exceptions {
    class HealthException: public DomainException {
        public:
            explicit HealthException(float health);

        private:
            static std::string buildMessage(float health);
    };
}
#endif /* !HEALTHEXCEPTION_HPP_ */
