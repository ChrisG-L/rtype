/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBFriendRequestRepository - MongoDB implementation for friend requests
*/

#include "infrastructure/adapters/out/persistence/MongoDBFriendRequestRepository.hpp"
#include "infrastructure/logging/Logger.hpp"
#include <mongocxx/options/find.hpp>

namespace infrastructure::adapters::out::persistence {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using application::ports::out::persistence::FriendRequestData;

MongoDBFriendRequestRepository::MongoDBFriendRequestRepository(
    std::shared_ptr<MongoDBConfiguration> mongoDB)
    : _mongoDB(mongoDB)
{
    // Create indexes for efficient queries
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    // Unique compound index to prevent duplicate requests
    auto uniqueIndex = make_document(
        kvp("from_email", 1),
        kvp("to_email", 1)
    );
    mongocxx::options::index indexOpts;
    indexOpts.unique(true);
    try {
        collection.create_index(uniqueIndex.view(), indexOpts);
    } catch (...) {}

    // Index for incoming requests lookup
    auto toIndex = make_document(kvp("to_email", 1));
    try {
        collection.create_index(toIndex.view());
    } catch (...) {}

    // Index for outgoing requests lookup
    auto fromIndex = make_document(kvp("from_email", 1));
    try {
        collection.create_index(fromIndex.view());
    } catch (...) {}
}

FriendRequestData MongoDBFriendRequestRepository::documentToRequest(
    const bsoncxx::document::view& doc)
{
    FriendRequestData data;

    if (doc["from_email"]) {
        data.fromEmail = std::string(doc["from_email"].get_string().value);
    }
    if (doc["to_email"]) {
        data.toEmail = std::string(doc["to_email"].get_string().value);
    }
    if (doc["from_display_name"]) {
        data.fromDisplayName = std::string(doc["from_display_name"].get_string().value);
    }
    if (doc["created_at"]) {
        auto ms = doc["created_at"].get_date().to_int64();
        data.createdAt = std::chrono::system_clock::time_point{
            std::chrono::milliseconds{ms}
        };
    }

    return data;
}

void MongoDBFriendRequestRepository::createRequest(
    const std::string& fromEmail,
    const std::string& toEmail,
    const std::string& fromDisplayName)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBFriendRequestRepository::createRequest - {} -> {}", fromEmail, toEmail);

    auto now = std::chrono::system_clock::now();
    auto doc = make_document(
        kvp("from_email", fromEmail),
        kvp("to_email", toEmail),
        kvp("from_display_name", fromDisplayName),
        kvp("created_at", bsoncxx::types::b_date{now})
    );

    try {
        collection.insert_one(doc.view());
        logger->info("Friend request created: {} -> {}", fromEmail, toEmail);
    } catch (const std::exception& e) {
        logger->warn("Friend request creation failed: {} -> {} - {}", fromEmail, toEmail, e.what());
    }
}

void MongoDBFriendRequestRepository::deleteRequest(
    const std::string& fromEmail, const std::string& toEmail)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBFriendRequestRepository::deleteRequest - {} -> {}", fromEmail, toEmail);

    auto filter = make_document(
        kvp("from_email", fromEmail),
        kvp("to_email", toEmail)
    );

    auto result = collection.delete_one(filter.view());
    if (result && result->deleted_count() > 0) {
        logger->info("Friend request deleted: {} -> {}", fromEmail, toEmail);
    }
}

bool MongoDBFriendRequestRepository::requestExists(
    const std::string& fromEmail, const std::string& toEmail)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto filter = make_document(
        kvp("from_email", fromEmail),
        kvp("to_email", toEmail)
    );

    auto result = collection.find_one(filter.view());
    return result.has_value();
}

std::optional<FriendRequestData> MongoDBFriendRequestRepository::getRequest(
    const std::string& fromEmail, const std::string& toEmail)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto filter = make_document(
        kvp("from_email", fromEmail),
        kvp("to_email", toEmail)
    );

    auto result = collection.find_one(filter.view());
    if (result) {
        return documentToRequest(result->view());
    }
    return std::nullopt;
}

std::vector<FriendRequestData> MongoDBFriendRequestRepository::getIncomingRequests(
    const std::string& email)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBFriendRequestRepository::getIncomingRequests - {}", email);

    std::vector<FriendRequestData> requests;

    auto filter = make_document(kvp("to_email", email));
    auto sort = make_document(kvp("created_at", -1));  // Newest first

    mongocxx::options::find opts;
    opts.sort(sort.view());

    auto cursor = collection.find(filter.view(), opts);
    for (auto&& doc : cursor) {
        requests.push_back(documentToRequest(doc));
    }

    logger->debug("MongoDBFriendRequestRepository::getIncomingRequests - Found {} requests", requests.size());
    return requests;
}

std::vector<FriendRequestData> MongoDBFriendRequestRepository::getOutgoingRequests(
    const std::string& email)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBFriendRequestRepository::getOutgoingRequests - {}", email);

    std::vector<FriendRequestData> requests;

    auto filter = make_document(kvp("from_email", email));
    auto sort = make_document(kvp("created_at", -1));

    mongocxx::options::find opts;
    opts.sort(sort.view());

    auto cursor = collection.find(filter.view(), opts);
    for (auto&& doc : cursor) {
        requests.push_back(documentToRequest(doc));
    }

    logger->debug("MongoDBFriendRequestRepository::getOutgoingRequests - Found {} requests", requests.size());
    return requests;
}

size_t MongoDBFriendRequestRepository::getPendingRequestCount(const std::string& email)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto filter = make_document(kvp("to_email", email));
    return static_cast<size_t>(collection.count_documents(filter.view()));
}

} // namespace infrastructure::adapters::out::persistence
