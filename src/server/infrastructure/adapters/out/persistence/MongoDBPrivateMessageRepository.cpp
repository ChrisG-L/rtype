/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBPrivateMessageRepository - MongoDB implementation for private messages
*/

#include "infrastructure/adapters/out/persistence/MongoDBPrivateMessageRepository.hpp"
#include "infrastructure/logging/Logger.hpp"
#include <mongocxx/options/find.hpp>
#include <bsoncxx/types/bson_value/value.hpp>

namespace infrastructure::adapters::out::persistence {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using application::ports::out::persistence::PrivateMessageData;
using application::ports::out::persistence::ConversationSummaryData;

MongoDBPrivateMessageRepository::MongoDBPrivateMessageRepository(
    std::shared_ptr<MongoDBConfiguration> mongoDB)
    : _mongoDB(mongoDB)
{
    // Create indexes for efficient queries
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    // Index for conversation history
    auto convIndex = make_document(
        kvp("conversation_key", 1),
        kvp("timestamp", -1)
    );
    try {
        collection.create_index(convIndex.view());
    } catch (...) {}

    // Index for unread count
    auto unreadIndex = make_document(
        kvp("recipient_email", 1),
        kvp("is_read", 1)
    );
    try {
        collection.create_index(unreadIndex.view());
    } catch (...) {}

    // Index for cleanup old messages
    auto timestampIndex = make_document(kvp("timestamp", -1));
    try {
        collection.create_index(timestampIndex.view());
    } catch (...) {}
}

std::string MongoDBPrivateMessageRepository::makeConversationKey(
    const std::string& email1, const std::string& email2)
{
    if (email1 < email2) {
        return email1 + ":" + email2;
    }
    return email2 + ":" + email1;
}

PrivateMessageData MongoDBPrivateMessageRepository::documentToMessage(
    const bsoncxx::document::view& doc)
{
    PrivateMessageData data;

    if (doc["_id"]) {
        // Convert ObjectId to uint64 (use timestamp portion)
        auto oid = doc["_id"].get_oid().value;
        data.id = static_cast<uint64_t>(oid.get_time_t());
    }
    if (doc["sender_email"]) {
        data.senderEmail = std::string(doc["sender_email"].get_string().value);
    }
    if (doc["recipient_email"]) {
        data.recipientEmail = std::string(doc["recipient_email"].get_string().value);
    }
    if (doc["sender_display_name"]) {
        data.senderDisplayName = std::string(doc["sender_display_name"].get_string().value);
    }
    if (doc["message"]) {
        data.message = std::string(doc["message"].get_string().value);
    }
    if (doc["timestamp"]) {
        auto ms = doc["timestamp"].get_date().to_int64();
        data.timestamp = std::chrono::system_clock::time_point{
            std::chrono::milliseconds{ms}
        };
    }
    if (doc["is_read"]) {
        data.isRead = doc["is_read"].get_bool().value;
    }

    return data;
}

uint64_t MongoDBPrivateMessageRepository::saveMessage(
    const std::string& senderEmail,
    const std::string& recipientEmail,
    const std::string& senderDisplayName,
    const std::string& message)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBPrivateMessageRepository::saveMessage - {} -> {}",
                  senderEmail, recipientEmail);

    auto now = std::chrono::system_clock::now();
    auto convKey = makeConversationKey(senderEmail, recipientEmail);

    auto doc = make_document(
        kvp("conversation_key", convKey),
        kvp("sender_email", senderEmail),
        kvp("recipient_email", recipientEmail),
        kvp("sender_display_name", senderDisplayName),
        kvp("message", message),
        kvp("timestamp", bsoncxx::types::b_date{now}),
        kvp("is_read", false)
    );

    auto result = collection.insert_one(doc.view());
    if (result) {
        auto oid = result->inserted_id().get_oid().value;
        auto messageId = static_cast<uint64_t>(oid.get_time_t());
        logger->info("Private message saved: {} -> {} (id={})", senderEmail, recipientEmail, messageId);
        return messageId;
    }

    logger->error("Failed to save private message: {} -> {}", senderEmail, recipientEmail);
    return 0;
}

std::vector<PrivateMessageData> MongoDBPrivateMessageRepository::getConversation(
    const std::string& email1,
    const std::string& email2,
    size_t offset,
    size_t limit)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBPrivateMessageRepository::getConversation - {} <-> {}, offset={}, limit={}",
                  email1, email2, offset, limit);

    std::vector<PrivateMessageData> messages;

    auto convKey = makeConversationKey(email1, email2);
    auto filter = make_document(kvp("conversation_key", convKey));
    auto sort = make_document(kvp("timestamp", -1));  // Newest first

    mongocxx::options::find opts;
    opts.sort(sort.view());
    opts.skip(static_cast<int64_t>(offset));
    opts.limit(static_cast<int64_t>(limit));

    auto cursor = collection.find(filter.view(), opts);
    for (auto&& doc : cursor) {
        messages.push_back(documentToMessage(doc));
    }

    logger->debug("MongoDBPrivateMessageRepository::getConversation - Found {} messages", messages.size());
    return messages;
}

std::vector<ConversationSummaryData> MongoDBPrivateMessageRepository::getConversationsList(
    const std::string& email,
    size_t limit)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBPrivateMessageRepository::getConversationsList - {}", email);

    // Use aggregation to get unique conversations with last message
    auto pipeline = mongocxx::pipeline{};

    // Match messages where user is sender or recipient
    pipeline.match(make_document(
        kvp("$or", bsoncxx::builder::basic::make_array(
            make_document(kvp("sender_email", email)),
            make_document(kvp("recipient_email", email))
        ))
    ));

    // Sort by timestamp desc
    pipeline.sort(make_document(kvp("timestamp", -1)));

    // Group by conversation key
    pipeline.group(make_document(
        kvp("_id", "$conversation_key"),
        kvp("lastMessage", make_document(kvp("$first", "$message"))),
        kvp("lastTimestamp", make_document(kvp("$first", "$timestamp"))),
        kvp("senderEmail", make_document(kvp("$first", "$sender_email"))),
        kvp("recipientEmail", make_document(kvp("$first", "$recipient_email"))),
        kvp("senderDisplayName", make_document(kvp("$first", "$sender_display_name")))
    ));

    // Sort by last message timestamp
    pipeline.sort(make_document(kvp("lastTimestamp", -1)));

    // Limit results
    pipeline.limit(static_cast<int32_t>(limit));

    std::vector<ConversationSummaryData> conversations;

    auto cursor = collection.aggregate(pipeline);
    for (auto&& doc : cursor) {
        ConversationSummaryData summary;

        // Get the other user's email
        std::string senderEmail = std::string(doc["senderEmail"].get_string().value);
        std::string recipientEmail = std::string(doc["recipientEmail"].get_string().value);

        if (senderEmail == email) {
            summary.otherEmail = recipientEmail;
            // We don't have recipient display name stored, use email for now
            summary.otherDisplayName = recipientEmail;
        } else {
            summary.otherEmail = senderEmail;
            summary.otherDisplayName = std::string(doc["senderDisplayName"].get_string().value);
        }

        summary.lastMessage = std::string(doc["lastMessage"].get_string().value);
        auto ms = doc["lastTimestamp"].get_date().to_int64();
        summary.lastTimestamp = std::chrono::system_clock::time_point{
            std::chrono::milliseconds{ms}
        };

        // Get unread count for this conversation
        summary.unreadCount = static_cast<uint8_t>(
            getUnreadCountFrom(email, summary.otherEmail)
        );

        conversations.push_back(summary);
    }

    logger->debug("MongoDBPrivateMessageRepository::getConversationsList - Found {} conversations",
                  conversations.size());
    return conversations;
}

void MongoDBPrivateMessageRepository::markAsRead(
    const std::string& readerEmail,
    const std::string& senderEmail)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBPrivateMessageRepository::markAsRead - {} reads messages from {}",
                  readerEmail, senderEmail);

    auto filter = make_document(
        kvp("sender_email", senderEmail),
        kvp("recipient_email", readerEmail),
        kvp("is_read", false)
    );

    auto update = make_document(
        kvp("$set", make_document(kvp("is_read", true)))
    );

    auto result = collection.update_many(filter.view(), update.view());
    if (result) {
        logger->debug("Marked {} messages as read", result->modified_count());
    }
}

size_t MongoDBPrivateMessageRepository::getUnreadCount(const std::string& email)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto filter = make_document(
        kvp("recipient_email", email),
        kvp("is_read", false)
    );

    return static_cast<size_t>(collection.count_documents(filter.view()));
}

size_t MongoDBPrivateMessageRepository::getUnreadCountFrom(
    const std::string& recipientEmail,
    const std::string& senderEmail)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto filter = make_document(
        kvp("sender_email", senderEmail),
        kvp("recipient_email", recipientEmail),
        kvp("is_read", false)
    );

    return static_cast<size_t>(collection.count_documents(filter.view()));
}

// ============ Admin functions ============

std::vector<PrivateMessageData> MongoDBPrivateMessageRepository::getMessagesByUser(
    const std::string& email,
    size_t limit)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBPrivateMessageRepository::getMessagesByUser - {}", email);

    std::vector<PrivateMessageData> messages;

    // Messages where user is sender or recipient
    auto filter = make_document(
        kvp("$or", bsoncxx::builder::basic::make_array(
            make_document(kvp("sender_email", email)),
            make_document(kvp("recipient_email", email))
        ))
    );
    auto sort = make_document(kvp("timestamp", -1));

    mongocxx::options::find opts;
    opts.sort(sort.view());
    opts.limit(static_cast<int64_t>(limit));

    auto cursor = collection.find(filter.view(), opts);
    for (auto&& doc : cursor) {
        messages.push_back(documentToMessage(doc));
    }

    logger->debug("MongoDBPrivateMessageRepository::getMessagesByUser - Found {} messages", messages.size());
    return messages;
}

std::vector<PrivateMessageData> MongoDBPrivateMessageRepository::getAllMessages(
    size_t limit,
    uint64_t beforeTimestamp)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBPrivateMessageRepository::getAllMessages - limit={}, before={}",
                  limit, beforeTimestamp);

    std::vector<PrivateMessageData> messages;

    bsoncxx::builder::basic::document filter{};
    if (beforeTimestamp > 0) {
        auto beforeTime = std::chrono::system_clock::time_point{
            std::chrono::milliseconds{beforeTimestamp}
        };
        filter.append(kvp("timestamp", make_document(
            kvp("$lt", bsoncxx::types::b_date{beforeTime})
        )));
    }

    auto sort = make_document(kvp("timestamp", -1));

    mongocxx::options::find opts;
    opts.sort(sort.view());
    opts.limit(static_cast<int64_t>(limit));

    auto cursor = collection.find(filter.view(), opts);
    for (auto&& doc : cursor) {
        messages.push_back(documentToMessage(doc));
    }

    logger->debug("MongoDBPrivateMessageRepository::getAllMessages - Found {} messages", messages.size());
    return messages;
}

std::vector<PrivateMessageData> MongoDBPrivateMessageRepository::searchMessages(
    const std::string& searchTerm,
    size_t limit)
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    auto logger = server::logging::Logger::getMainLogger();
    logger->debug("MongoDBPrivateMessageRepository::searchMessages - term={}", searchTerm);

    std::vector<PrivateMessageData> messages;

    // Use regex for case-insensitive search
    auto filter = make_document(
        kvp("message", make_document(
            kvp("$regex", searchTerm),
            kvp("$options", "i")  // Case insensitive
        ))
    );
    auto sort = make_document(kvp("timestamp", -1));

    mongocxx::options::find opts;
    opts.sort(sort.view());
    opts.limit(static_cast<int64_t>(limit));

    auto cursor = collection.find(filter.view(), opts);
    for (auto&& doc : cursor) {
        messages.push_back(documentToMessage(doc));
    }

    logger->debug("MongoDBPrivateMessageRepository::searchMessages - Found {} messages", messages.size());
    return messages;
}

std::pair<size_t, size_t> MongoDBPrivateMessageRepository::getMessageStats()
{
    auto client = _mongoDB->acquireClient();
    auto db = _mongoDB->getDatabase(client);
    auto collection = db[COLLECTION_NAME];

    // Total messages
    auto totalMessages = static_cast<size_t>(collection.count_documents({}));

    // Unique conversations (distinct conversation_key)
    auto pipeline = mongocxx::pipeline{};
    pipeline.group(make_document(
        kvp("_id", "$conversation_key")
    ));
    pipeline.count("count");

    size_t totalConversations = 0;
    auto cursor = collection.aggregate(pipeline);
    for (auto&& doc : cursor) {
        if (doc["count"]) {
            totalConversations = static_cast<size_t>(doc["count"].get_int32().value);
        }
    }

    return {totalMessages, totalConversations};
}

} // namespace infrastructure::adapters::out::persistence
