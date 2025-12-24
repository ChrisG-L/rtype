/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoDBException - MongoDB-specific persistence exception
*/

#ifndef INFRASTRUCTURE_MONGODBEXCEPTION_HPP_
#define INFRASTRUCTURE_MONGODBEXCEPTION_HPP_

#include "domain/exceptions/persistence/PersistenceException.hpp"

namespace infrastructure::exceptions {
    class MongoDBException: public domain::exceptions::persistence::PersistenceException {
        public:
            explicit MongoDBException()
                : PersistenceException("The mongoDB server is not reachable. Please check your connection settings and ensure the server is running.") {}

            explicit MongoDBException(const std::string& message)
                : PersistenceException(message) {}
    };
}

#endif /* !INFRASTRUCTURE_MONGODBEXCEPTION_HPP_ */
