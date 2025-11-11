/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoDBException
*/

#include "domain/exceptions/persistence/MongoDBException.hpp"
#include <sstream>

namespace domain::exceptions::persistence {
    MongoDBException::MongoDBException(): DomainException(buildMessage())
    {
    }

    std::string MongoDBException::buildMessage() {
        std::ostringstream oss;
        oss << "The mongoDB server is not reachable. Please check your connection settings and ensure the server is running.";
        return oss.str();
    }
}
