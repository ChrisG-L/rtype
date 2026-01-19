/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBBlockedUserRepository - MongoDB implementation for blocked users
*/

#include "infrastructure/adapters/out/persistence/MongoDBBlockedUserRepository.hpp"
#include "infrastructure/logging/Logger.hpp"
#include <mongocxx/options/find.hpp>

namespace infrastructure::adapters::out::persistence {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using application::ports::out::persistence::BlockedUserData;

MongoDBBlockedUserRepository::MongoDBBlockedUserRepository(
    std::shared_ptr<MongoDBConfiguration> mongoDB)
    : _mongoDB(mongoDB)
{
    // Create indexes for efficient queries
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    // Unique compound index to prevent duplicate blocks
    auto uniqueIndex = make_document(
        kvp("blocker_email", 1),
        kvp("blocked_email", 1)
    );
    mongocxx::options::index indexOpts;
    indexOpts.unique(true);
    try {
        collection.create_index(uniqueIndex.view(), indexOpts);
    } catch (...) {}

    // Index for checking if someone is blocked by anyone
    auto blockedIndex = make_document(kvp("blocked_email", 1));
    try {
        collection.create_index(blockedIndex.view());
    } catch (...) {}
}

BlockedUserData MongoDBBlockedUserRepository::documentToBlockedUser(
    const bsoncxx::document::view& doc)
{
    BlockedUserData data;

    if (doc["blocked_email"]) {
        data.blockedEmail = std::string(doc["blocked_email"].get_string().value);
    }
    if (doc["blocked_display_name"]) {
        data.blockedDisplayName = std::string(doc["blocked_display_name"].get_string().value);
    }
    if (doc["created_at"]) {
        auto ms = doc["created_at"].get_date().to_int64();
        data.createdAt = std::chrono::system_clock::time_point{
            std::chrono::milliseconds{ms}
        };
    }

    return data;
}

void MongoDBBlockedUserRepository::blockUser(
    const std::string& blockerEmail,
    const std::string& blockedEmail,
    const std::string& blockedDisplayName)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBBlockedUserRepository::blockUser - {} blocks {}", blockerEmail, blockedEmail);

    auto now = std::chrono::system_clock::now();
    auto doc = make_document(
        kvp("blocker_email", blockerEmail),
        kvp("blocked_email", blockedEmail),
        kvp("blocked_display_name", blockedDisplayName),
        kvp("created_at", bsoncxx::types::b_date{now})
    );

    try {
        collection.insert_one(doc.view());
        logger->info("User blocked: {} blocked {}", blockerEmail, blockedEmail);
    } catch (const std::exception& e) {
        logger->warn("Block user failed (may already exist): {} blocks {} - {}",
                     blockerEmail, blockedEmail, e.what());
    }
}

void MongoDBBlockedUserRepository::unblockUser(
    const std::string& blockerEmail,
    const std::string& blockedEmail)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBBlockedUserRepository::unblockUser - {} unblocks {}", blockerEmail, blockedEmail);

    auto filter = make_document(
        kvp("blocker_email", blockerEmail),
        kvp("blocked_email", blockedEmail)
    );

    auto result = collection.delete_one(filter.view());
    if (result && result->deleted_count() > 0) {
        logger->info("User unblocked: {} unblocked {}", blockerEmail, blockedEmail);
    }
}

bool MongoDBBlockedUserRepository::isBlocked(
    const std::string& blockerEmail,
    const std::string& blockedEmail)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto filter = make_document(
        kvp("blocker_email", blockerEmail),
        kvp("blocked_email", blockedEmail)
    );

    auto result = collection.find_one(filter.view());
    return result.has_value();
}

bool MongoDBBlockedUserRepository::hasAnyBlock(
    const std::string& email1, const std::string& email2)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    // Check if either user has blocked the other
    auto filter = make_document(
        kvp("$or", bsoncxx::builder::basic::make_array(
            make_document(
                kvp("blocker_email", email1),
                kvp("blocked_email", email2)
            ),
            make_document(
                kvp("blocker_email", email2),
                kvp("blocked_email", email1)
            )
        ))
    );

    auto result = collection.find_one(filter.view());
    return result.has_value();
}

std::vector<BlockedUserData> MongoDBBlockedUserRepository::getBlockedUsers(
    const std::string& blockerEmail)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBBlockedUserRepository::getBlockedUsers - {}", blockerEmail);

    std::vector<BlockedUserData> blockedUsers;

    auto filter = make_document(kvp("blocker_email", blockerEmail));
    auto sort = make_document(kvp("created_at", -1));

    mongocxx::options::find opts;
    opts.sort(sort.view());

    auto cursor = collection.find(filter.view(), opts);
    for (auto&& doc : cursor) {
        blockedUsers.push_back(documentToBlockedUser(doc));
    }

    logger->debug("MongoDBBlockedUserRepository::getBlockedUsers - Found {} blocked users",
                  blockedUsers.size());
    return blockedUsers;
}

} // namespace infrastructure::adapters::out::persistence
