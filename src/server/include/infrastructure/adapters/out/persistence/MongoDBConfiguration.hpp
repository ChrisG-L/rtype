/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoDBConfiguration
*/

#ifndef MONGODBCONFIGURATION_HPP_
#define MONGODBCONFIGURATION_HPP_

#include "../../../configuration/DBConfig.hpp"
#include "domain/exceptions/persistence/MongoDBException.hpp"

#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/builder/basic/document.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

#include <memory>

namespace infrastructure::adapters::out::persistence {
    class MongoDBConfiguration {
        private:
            static std::unique_ptr<mongocxx::instance> _instance;
            DBConfig _dbConfig;
            mongocxx::uri _uri;
            mongocxx::client _client;

        public:
            explicit MongoDBConfiguration(const DBConfig& dbconfig);
            mongocxx::database getDatabaseConfig() const;
            const DBConfig& getConfig() const;
            bool pingServer() const;
    };
}

#endif /* !MONGODBCONFIGURATION_HPP_ */
