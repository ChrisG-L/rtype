/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoDBConfiguration
*/

#include "infrastructure/adapters/out/persistence/MongoDBConfiguration.hpp"

namespace infrastructure::adapters::out::persistence {
    std::unique_ptr<mongocxx::instance> MongoDBConfiguration::_instance = nullptr;

    MongoDBConfiguration::MongoDBConfiguration(const DBConfig& dbconfig)
        : _dbConfig(dbconfig), _uri(_dbConfig.connexionString), _client(_uri) {
        if (!_instance) {
            _instance = std::make_unique<mongocxx::instance>();
        }
        if (!pingServer()) {
            throw domain::exceptions::persistence::MongoDBException();
        }
    }

    mongocxx::database MongoDBConfiguration::getDatabaseConfig() const {
        return _client[_dbConfig.dbName];
    }

    const DBConfig& MongoDBConfiguration::getConfig() const {
        return _dbConfig;
    }

    bool MongoDBConfiguration::pingServer() const {
        try {
            auto db = getDatabaseConfig();
            auto collection = db["ping"];
            auto result = collection.find_one(make_document(kvp("ping", "me")));

            if (result) {
                std::cout << bsoncxx::to_json(*result) << std::endl;
                return true;
            } else {
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Erreur de ping MongoDB: " << e.what() << std::endl;
            return false;
        }
    };
}
