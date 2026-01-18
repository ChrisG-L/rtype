/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBChatMessageRepository - MongoDB implementation for chat messages
*/

#include "infrastructure/adapters/out/persistence/MongoDBChatMessageRepository.hpp"
#include "infrastructure/logging/Logger.hpp"
#include <mongocxx/options/find.hpp>

namespace infrastructure::adapters::out::persistence {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

MongoDBChatMessageRepository::MongoDBChatMessageRepository(
    std::shared_ptr<MongoDBConfiguration> mongoDB)
    : _mongoDB(mongoDB)
{
    // Create index for efficient queries (room_code + timestamp)
    // This is idempotent - MongoDB won't create duplicate indexes
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto indexKeys = make_document(
        kvp("room_code", 1),
        kvp("timestamp", 1)
    );
    collection.create_index(indexKeys.view());
}

bsoncxx::types::b_date MongoDBChatMessageRepository::timePointToDate(
    const std::chrono::system_clock::time_point& tp)
{
    return bsoncxx::types::b_date{tp};
}

std::chrono::system_clock::time_point MongoDBChatMessageRepository::dateToTimePoint(
    const bsoncxx::types::b_date& date)
{
    return std::chrono::system_clock::time_point{
        std::chrono::milliseconds{date.to_int64()}
    };
}

ChatMessageData MongoDBChatMessageRepository::documentToMessage(
    const bsoncxx::document::view& doc)
{
    ChatMessageData msg;

    if (doc["room_code"]) {
        msg.roomCode = std::string(doc["room_code"].get_string().value);
    }
    if (doc["display_name"]) {
        msg.displayName = std::string(doc["display_name"].get_string().value);
    }
    if (doc["message"]) {
        msg.message = std::string(doc["message"].get_string().value);
    }
    if (doc["timestamp"]) {
        msg.timestamp = dateToTimePoint(doc["timestamp"].get_date());
    }

    return msg;
}

void MongoDBChatMessageRepository::save(const ChatMessageData& message)
{
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBChatMessageRepository::save - room_code={}, display_name={}, message={}",
                  message.roomCode, message.displayName, message.message);

    auto doc = make_document(
        kvp("room_code", message.roomCode),
        kvp("display_name", message.displayName),
        kvp("message", message.message),
        kvp("timestamp", timePointToDate(message.timestamp))
    );

    auto result = collection.insert_one(doc.view());
    if (result) {
        logger->debug("MongoDBChatMessageRepository::save - Insert successful");
    } else {
        logger->error("MongoDBChatMessageRepository::save - Insert failed!");
    }
}

std::vector<ChatMessageData> MongoDBChatMessageRepository::findByRoomCode(
    const std::string& roomCode, size_t limit)
{
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBChatMessageRepository::findByRoomCode - room_code={}, limit={}", roomCode, limit);

    std::vector<ChatMessageData> messages;

    auto filter = make_document(kvp("room_code", roomCode));
    auto sort = make_document(kvp("timestamp", 1));  // Ascending order (oldest first)

    mongocxx::options::find opts;
    opts.sort(sort.view());
    opts.limit(static_cast<int64_t>(limit));

    auto cursor = collection.find(filter.view(), opts);
    for (auto&& doc : cursor) {
        messages.push_back(documentToMessage(doc));
    }

    logger->debug("MongoDBChatMessageRepository::findByRoomCode - Found {} messages", messages.size());
    return messages;
}

bool MongoDBChatMessageRepository::hasHistoryForCode(const std::string& roomCode)
{
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto filter = make_document(kvp("room_code", roomCode));
    auto count = collection.count_documents(filter.view());
    return count > 0;
}

void MongoDBChatMessageRepository::deleteByRoomCode(const std::string& roomCode)
{
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto filter = make_document(kvp("room_code", roomCode));
    collection.delete_many(filter.view());
}

void MongoDBChatMessageRepository::deleteOldestRoomHistory()
{
    // Acquire client from pool (thread-safe) - stays alive for this method
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    // Find the oldest message across all rooms
    auto sort = make_document(kvp("timestamp", 1));  // Ascending (oldest first)

    mongocxx::options::find opts;
    opts.sort(sort.view());
    opts.limit(1);

    auto cursor = collection.find(make_document().view(), opts);
    for (auto&& doc : cursor) {
        if (doc["room_code"]) {
            std::string oldestRoomCode = std::string(doc["room_code"].get_string().value);
            // Delete all messages for this room (uses its own pooled connection)
            deleteByRoomCode(oldestRoomCode);
            return;
        }
    }
}

} // namespace infrastructure::adapters::out::persistence
