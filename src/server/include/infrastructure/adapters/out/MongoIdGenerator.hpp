/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoIdGenerator - MongoDB ObjectId generator adapter
*/

#ifndef MONGOIDGENERATOR_HPP_
#define MONGOIDGENERATOR_HPP_

#include "application/ports/out/IIdGenerator.hpp"
#include <bsoncxx/oid.hpp>

namespace infrastructure::adapters::out {
    class MongoIdGenerator : public application::ports::out::IIdGenerator {
        public:
            std::string generate() override {
                return bsoncxx::oid().to_string();
            }
    };
}

#endif /* !MONGOIDGENERATOR_HPP_ */
