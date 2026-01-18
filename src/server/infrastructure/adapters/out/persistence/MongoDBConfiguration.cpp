/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoDBConfiguration
*/

#include "infrastructure/adapters/out/persistence/MongoDBConfiguration.hpp"
#include "infrastructure/logging/Logger.hpp"

namespace infrastructure::adapters::out::persistence {
    std::unique_ptr<mongocxx::instance> MongoDBConfiguration::_instance = nullptr;

    MongoDBConfiguration::MongoDBConfiguration(const DBConfig& dbconfig)
        : _dbConfig(dbconfig), _uri(_dbConfig.connexionString) {
        if (!_instance) {
            _instance = std::make_unique<mongocxx::instance>();
        }

        // Create connection pool with configured pool size
        // The URI can include pool options: ?minPoolSize=X&maxPoolSize=Y
        mongocxx::options::pool poolOptions;
        // Note: Pool size is typically controlled via URI parameters
        // e.g., mongodb://localhost:27017/?minPoolSize=1&maxPoolSize=10
        _pool = std::make_unique<mongocxx::pool>(_uri, poolOptions);

        if (!pingServer()) {
            throw infrastructure::exceptions::MongoDBException();
        }

        server::logging::Logger::getMainLogger()->info(
            "MongoDB connection pool initialized (thread-safe mode)");
    }

    PooledClient MongoDBConfiguration::acquireClient() {
        // This is thread-safe - each thread gets its own client from the pool
        return _pool->acquire();
    }

    mongocxx::database MongoDBConfiguration::getDatabase(PooledClient& client) const {
        return (*client)[_dbConfig.dbName];
    }

    mongocxx::database MongoDBConfiguration::getDatabaseConfig() const {
        // Legacy API for backward compatibility
        // Acquires a temporary client from the pool
        // WARNING: This creates a new client for each call - use acquireClient() for efficiency
        auto client = const_cast<MongoDBConfiguration*>(this)->_pool->acquire();
        return (*client)[_dbConfig.dbName];
    }

    const DBConfig& MongoDBConfiguration::getConfig() const {
        return _dbConfig;
    }

    bool MongoDBConfiguration::pingServer() const {
        try {
            auto client = const_cast<MongoDBConfiguration*>(this)->_pool->acquire();
            auto db = (*client)["admin"];
            auto result = db.run_command(make_document(kvp("ping", 1)));
            return true;
        } catch (const std::exception& e) {
            server::logging::Logger::getMainLogger()->error("Erreur de ping MongoDB: {}", e.what());
            return false;
        }
    }
}
