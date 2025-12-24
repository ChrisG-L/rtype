/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PersistenceException - Generic persistence layer exception
*/

#ifndef PERSISTENCEEXCEPTION_HPP_
#define PERSISTENCEEXCEPTION_HPP_

#include "../DomainException.hpp"

namespace domain::exceptions::persistence {
    class PersistenceException: public DomainException {
        public:
            explicit PersistenceException(const std::string& message)
                : DomainException(message) {}
    };
}

#endif /* !PERSISTENCEEXCEPTION_HPP_ */
