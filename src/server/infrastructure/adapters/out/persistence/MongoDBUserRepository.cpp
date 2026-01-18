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

    MongoDBUserRepository::MongoDBUserRepository(std::shared_ptr<MongoDBConfiguration> mongoDB)
        : _mongoDB(mongoDB)
    {
        // No longer store collection - acquire from pool for each operation
    }

    MongoDBUserRepository::~MongoDBUserRepository() = default;

    mongocxx::collection MongoDBUserRepository::getCollection() {
        // Note: This acquires a temporary client. The caller must ensure
        // the client stays alive by calling acquireClient() separately.
        // This method is kept for backward compatibility but should not be used.
        auto client = _mongoDB->acquireClient();
        return _mongoDB->getDatabase(client)[COLLECTION_NAME];
    }

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
        // Acquire client from pool (thread-safe) - stays alive for this method
        auto client = const_cast<MongoDBUserRepository*>(this)->_mongoDB->acquireClient();
        auto db = const_cast<MongoDBUserRepository*>(this)->_mongoDB->getDatabase(client);
        auto collection = db[COLLECTION_NAME];

        auto isUserExist = collection.find_one(make_document(kvp("username", user.getUsername().value())));
        if (isUserExist.has_value()) {
            throw domain::exceptions::user::UsernameAlreadyExistsException(user.getUsername().value());
        }
        auto isEmailExist = collection.find_one(make_document(kvp("email", user.getEmail().value())));
        if (isEmailExist.has_value()) {
            throw domain::exceptions::user::EmailAlreadyExistsException(user.getEmail().value());
        }
        auto user_doc = make_document(
            kvp("username", user.getUsername().value()),
            kvp("email", user.getEmail().value()),
            kvp("password", user.getPasswordHash().value()),
            kvp("lastLogin", timePointToDate(user.getLastLogin())),
            kvp("createAt", timePointToDate(user.getCreatedAt())));
        collection.insert_one(user_doc.view());
    }

    void MongoDBUserRepository::update(const User& user) {
        // Acquire client from pool (thread-safe) - stays alive for this method
        auto client = _mongoDB->acquireClient();
        auto db = _mongoDB->getDatabase(client);
        auto collection = db[COLLECTION_NAME];

        // Find by ObjectId and update the fields
        bsoncxx::oid oid(user.getId().value());

        auto filter = make_document(kvp("_id", oid));
        auto update = make_document(
            kvp("$set", make_document(
                kvp("username", user.getUsername().value()),
                kvp("email", user.getEmail().value()),
                kvp("password", user.getPasswordHash().value()),
                kvp("lastLogin", timePointToDate(user.getLastLogin()))
            ))
        );

        collection.update_one(filter.view(), update.view());
    }

    std::optional<User> MongoDBUserRepository::findById(const std::string& id) {
        // Acquire client from pool (thread-safe) - stays alive for this method
        auto client = _mongoDB->acquireClient();
        auto db = _mongoDB->getDatabase(client);
        auto collection = db[COLLECTION_NAME];

        std::optional<User> user;
        auto result = collection.find_one(make_document(kvp("_id", id)));
        if (result.has_value())
            user = documentToUser(result->view());
        return user;
    }

    std::optional<User> MongoDBUserRepository::findByName(const std::string& username) {
        // Acquire client from pool (thread-safe) - stays alive for this method
        auto client = _mongoDB->acquireClient();
        auto db = _mongoDB->getDatabase(client);
        auto collection = db[COLLECTION_NAME];

        std::optional<User> user;
        auto result = collection.find_one(make_document(kvp("username", username)));
        if (result.has_value())
            user = documentToUser(result->view());
        return user;
    }

    std::optional<User> MongoDBUserRepository::findByEmail(const std::string& email) {
        // Acquire client from pool (thread-safe) - stays alive for this method
        auto client = _mongoDB->acquireClient();
        auto db = _mongoDB->getDatabase(client);
        auto collection = db[COLLECTION_NAME];

        std::optional<User> user;
        auto result = collection.find_one(make_document(kvp("email", email)));
        if (result.has_value())
            user = documentToUser(result->view());
        return user;
    }

    std::vector<User> MongoDBUserRepository::findAll() {
        // Acquire client from pool (thread-safe) - stays alive for this method
        auto client = _mongoDB->acquireClient();
        auto db = _mongoDB->getDatabase(client);
        auto collection = db[COLLECTION_NAME];

        std::vector<User> users;
        auto cursor = collection.find({});
        for (auto&& doc : cursor) {
            users.push_back(documentToUser(doc));
        }
        return users;
    }
}
