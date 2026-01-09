/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoDBUserRepository
*/

#include "infrastructure/adapters/out/persistence/MongoDBUserRepository.hpp"
#include "domain/exceptions/user/UsernameAlreadyExistsException.hpp"
#include "domain/exceptions/user/EmailAlreadyExistsException.hpp"

namespace infrastructure::adapters::out::persistence {
    static std::unique_ptr<mongocxx::instance> _instance;
    std::shared_ptr<MongoDBConfiguration> MongoDBUserRepository::_mongoDB = nullptr;

    MongoDBUserRepository::MongoDBUserRepository(std::shared_ptr<MongoDBConfiguration> mongoDB)
    {
        _mongoDB = mongoDB;
        auto db = _mongoDB->getDatabaseConfig();
        _collection = std::make_unique<mongocxx::v_noabi::collection>(db["user"]);
    }

    MongoDBUserRepository::~MongoDBUserRepository() = default;

    bsoncxx::types::b_date MongoDBUserRepository::timePointToDate(
        const std::chrono::system_clock::time_point& tp) const {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp.time_since_epoch()
        );
        return bsoncxx::types::b_date{ms};
    }

    User MongoDBUserRepository::documentToUser(const bsoncxx::document::view& doc) {
        auto id = doc["_id"].get_oid().value.to_string();
        auto username = doc["username"].get_string().value;
        auto email = doc["email"].get_string().value;
        auto password = doc["password"].get_string().value;

        auto lastLoginMs = doc["lastLogin"].get_date().value;
        auto lastLogin = std::chrono::system_clock::time_point{
            std::chrono::milliseconds{lastLoginMs.count()}
        };
        auto createdAtMs = doc["createAt"].get_date().value;
        auto createdAt = std::chrono::system_clock::time_point{
            std::chrono::milliseconds{createdAtMs.count()}
        };

        return User(
            domain::value_objects::user::UserId(std::string(id)),
            domain::value_objects::user::Username(std::string(username)),
            domain::value_objects::user::Email(std::string(email)),
            domain::value_objects::user::Password(std::string(password)),
            lastLogin,
            createdAt);
    }

    void MongoDBUserRepository::save(const User& user) const {
        auto isUserExist = _collection->find_one(make_document(kvp("username", user.getUsername().value())));
        if (isUserExist.has_value()) {
            throw domain::exceptions::user::UsernameAlreadyExistsException(user.getUsername().value());
        }
        auto isEmailExist = _collection->find_one(make_document(kvp("email", user.getEmail().value())));
        if (isEmailExist.has_value()) {
            throw domain::exceptions::user::EmailAlreadyExistsException(user.getEmail().value());
        }
        auto user_doc = make_document(
            kvp("username", user.getUsername().value()),
            kvp("email", user.getEmail().value()),
            kvp("password", user.getPasswordHash().value()),
            kvp("lastLogin", timePointToDate(user.getLastLogin())),
            kvp("createAt", timePointToDate(user.getCreatedAt())));
        _collection->insert_one(user_doc.view());
    }

    void MongoDBUserRepository::update(const User& user) {
        // Non implémenté pour le moment
    }

    std::optional<User> MongoDBUserRepository::findById(const std::string& id) {
        std::optional<User> user;
        auto result = _collection->find_one(make_document(kvp("_id", id)));
        if (result.has_value())
            user = documentToUser(result->view());
        return user;
    }

    std::optional<User> MongoDBUserRepository::findByName(const std::string& username) {
        std::optional<User> user;
        auto result = _collection->find_one(make_document(kvp("username", username)));
        if (result.has_value())
            user = documentToUser(result->view());
        return user;
    }

    std::optional<User> MongoDBUserRepository::findByEmail(const std::string& email) {
        std::optional<User> user;
        auto result = _collection->find_one(make_document(kvp("email", email)));
        if (result.has_value())
            user = documentToUser(result->view());
        return user;
    }

    std::vector<User> MongoDBUserRepository::findAll() {
        std::vector<User> users;
        auto cursor = _collection->find({});
        for (auto&& doc : cursor) {
            users.push_back(documentToUser(doc));
        }
        return users;
    }
}

