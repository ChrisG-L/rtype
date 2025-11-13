/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoDBUserRepository
*/

#include "MongoDBUserRepository.hpp"

namespace infrastructure::adapters::out::persistence {
    MongoDBUserRepository::MongoDBUserRepository(std::unique_ptr<MongoDBConfiguration> mongoDB)
    {
        _mongoDB = std::make_shared<MongoDBConfiguration>(mongoDB);
        auto db = _mongoDB->getDatabaseConfig();
        _collection = std::make_unique<mongocxx::v_noabi::collection>(db["user"]);
    }

    MongoDBUserRepository::~MongoDBUserRepository()
    {
    }
    static std::unique_ptr<mongocxx::instance> _instance;
            DBConfig _dbConfig;
            mongocxx::uri _uri;
            mongocxx::client _client;

    User MongoDBUserRepository::documentToUser(const bsoncxx::document::view& doc) {

        auto id = doc["_id"].get_string().value;
    }

    void MongoDBUserRepository::save(const User& user) const {
        auto user_doc = make_document(
            kvp("username", user.getUsername()),
            kvp("password", user.getPasswordHash()),
            kvp("lastLogin", user.getLastLogin()),
            kvp("createAt", user.getCreatedAt()));
        _collection->insert_one(user_doc.view());
    }

    void MongoDBUserRepository::update(const User& user) {

    }

    std::optional<User> MongoDBUserRepository::findById(const std::string& id) const {
        auto result = _collection->find_one(make_document(kvp("_id", id)));
        // return result.value();
    }

    std::optional<User> MongoDBUserRepository::findByName(const std::string& name) const {

    }

    std::vector<User>MongoDBUserRepository::findAll() {

    }
}

