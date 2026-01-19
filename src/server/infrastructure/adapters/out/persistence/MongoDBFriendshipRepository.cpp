/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBFriendshipRepository - MongoDB implementation for friendships
*/

#include "infrastructure/adapters/out/persistence/MongoDBFriendshipRepository.hpp"
#include "infrastructure/logging/Logger.hpp"
#include <mongocxx/options/find.hpp>

namespace infrastructure::adapters::out::persistence {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

MongoDBFriendshipRepository::MongoDBFriendshipRepository(
    std::shared_ptr<MongoDBConfiguration> mongoDB)
    : _mongoDB(mongoDB)
{
    // Create indexes for efficient queries
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    // Unique compound index on ordered emails
    auto uniqueIndex = make_document(
        kvp("user1_email", 1),
        kvp("user2_email", 1)
    );
    mongocxx::options::index indexOpts;
    indexOpts.unique(true);
    try {
        collection.create_index(uniqueIndex.view(), indexOpts);
    } catch (...) {
        // Index may already exist
    }

    // Secondary index for reverse lookups
    auto secondaryIndex = make_document(kvp("user2_email", 1));
    try {
        collection.create_index(secondaryIndex.view());
    } catch (...) {
        // Index may already exist
    }
}

std::pair<std::string, std::string> MongoDBFriendshipRepository::orderEmails(
    const std::string& email1, const std::string& email2)
{
    if (email1 < email2) {
        return {email1, email2};
    }
    return {email2, email1};
}

void MongoDBFriendshipRepository::addFriendship(
    const std::string& email1, const std::string& email2)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    auto [user1, user2] = orderEmails(email1, email2);

    logger->debug("MongoDBFriendshipRepository::addFriendship - {} <-> {}", user1, user2);

    auto now = std::chrono::system_clock::now();
    auto doc = make_document(
        kvp("user1_email", user1),
        kvp("user2_email", user2),
        kvp("created_at", bsoncxx::types::b_date{now})
    );

    try {
        collection.insert_one(doc.view());
        logger->info("Friendship created: {} <-> {}", user1, user2);
    } catch (const std::exception& e) {
        logger->warn("Friendship already exists or error: {} <-> {} - {}", user1, user2, e.what());
    }
}

void MongoDBFriendshipRepository::removeFriendship(
    const std::string& email1, const std::string& email2)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    auto [user1, user2] = orderEmails(email1, email2);

    logger->debug("MongoDBFriendshipRepository::removeFriendship - {} <-> {}", user1, user2);

    auto filter = make_document(
        kvp("user1_email", user1),
        kvp("user2_email", user2)
    );

    auto result = collection.delete_one(filter.view());
    if (result && result->deleted_count() > 0) {
        logger->info("Friendship removed: {} <-> {}", user1, user2);
    } else {
        logger->debug("No friendship found to remove: {} <-> {}", user1, user2);
    }
}

bool MongoDBFriendshipRepository::areFriends(
    const std::string& email1, const std::string& email2)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto [user1, user2] = orderEmails(email1, email2);

    auto filter = make_document(
        kvp("user1_email", user1),
        kvp("user2_email", user2)
    );

    auto result = collection.find_one(filter.view());
    return result.has_value();
}

std::vector<std::string> MongoDBFriendshipRepository::getFriendEmails(
    const std::string& email, size_t offset, size_t limit)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBFriendshipRepository::getFriendEmails - email={}, offset={}, limit={}",
                  email, offset, limit);

    std::vector<std::string> friends;

    // Query where user is user1 or user2
    auto filter = make_document(
        kvp("$or", bsoncxx::builder::basic::make_array(
            make_document(kvp("user1_email", email)),
            make_document(kvp("user2_email", email))
        ))
    );

    mongocxx::options::find opts;
    opts.skip(static_cast<int64_t>(offset));
    opts.limit(static_cast<int64_t>(limit));

    auto cursor = collection.find(filter.view(), opts);
    for (auto&& doc : cursor) {
        std::string user1 = std::string(doc["user1_email"].get_string().value);
        std::string user2 = std::string(doc["user2_email"].get_string().value);

        // Return the other user's email
        if (user1 == email) {
            friends.push_back(user2);
        } else {
            friends.push_back(user1);
        }
    }

    logger->debug("MongoDBFriendshipRepository::getFriendEmails - Found {} friends", friends.size());
    return friends;
}

size_t MongoDBFriendshipRepository::getFriendCount(const std::string& email)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto filter = make_document(
        kvp("$or", bsoncxx::builder::basic::make_array(
            make_document(kvp("user1_email", email)),
            make_document(kvp("user2_email", email))
        ))
    );

    return static_cast<size_t>(collection.count_documents(filter.view()));
}

} // namespace infrastructure::adapters::out::persistence
