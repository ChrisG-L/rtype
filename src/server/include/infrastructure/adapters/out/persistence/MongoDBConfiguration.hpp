/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoDBConfiguration
*/

#ifndef MONGODBCONFIGURATION_HPP_
#define MONGODBCONFIGURATION_HPP_

#include "../../../configuration/DBConfig.hpp"
#include "infrastructure/exceptions/MongoDBException.hpp"

#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/builder/basic/document.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

#include <memory>
#include <mutex>

namespace infrastructure::adapters::out::persistence {

    // RAII wrapper for a pooled client entry
    // Automatically returns the client to the pool when destroyed
    using PooledClient = mongocxx::pool::entry;

    class MongoDBConfiguration {
        private:
            static std::unique_ptr<mongocxx::instance> _instance;
            DBConfig _dbConfig;
            mongocxx::uri _uri;
            std::unique_ptr<mongocxx::pool> _pool;  // Thread-safe connection pool

        public:
            explicit MongoDBConfiguration(const DBConfig& dbconfig);

            // Get a client from the pool (thread-safe)
            // The returned PooledClient automatically returns to the pool when destroyed
            PooledClient acquireClient();

            // Get database from a pooled client
            mongocxx::database getDatabase(PooledClient& client) const;

            // Legacy API - acquires a client internally (for backward compatibility)
            // WARNING: Not efficient for multiple operations - prefer acquireClient()
            mongocxx::database getDatabaseConfig() const;

            const DBConfig& getConfig() const;
            bool pingServer() const;
    };
}

#endif /* !MONGODBCONFIGURATION_HPP_ */
