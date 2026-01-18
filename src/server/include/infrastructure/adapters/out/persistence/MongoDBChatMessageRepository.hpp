/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBChatMessageRepository - MongoDB implementation for chat messages
*/

#ifndef MONGODBCHATMESSAGEREPOSITORY_HPP_
#define MONGODBCHATMESSAGEREPOSITORY_HPP_

#include "application/ports/out/persistence/IChatMessageRepository.hpp"
#include "MongoDBConfiguration.hpp"

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/types.hpp>

namespace infrastructure::adapters::out::persistence {

using application::ports::out::persistence::IChatMessageRepository;
using application::ports::out::persistence::ChatMessageData;

class MongoDBChatMessageRepository : public IChatMessageRepository {
private:
    std::shared_ptr<MongoDBConfiguration> _mongoDB;
    static constexpr const char* COLLECTION_NAME = "chat_messages";

    ChatMessageData documentToMessage(const bsoncxx::document::view& doc);
    bsoncxx::types::b_date timePointToDate(const std::chrono::system_clock::time_point& tp);
    std::chrono::system_clock::time_point dateToTimePoint(const bsoncxx::types::b_date& date);

public:
    explicit MongoDBChatMessageRepository(std::shared_ptr<MongoDBConfiguration> mongoDB);
    ~MongoDBChatMessageRepository() override = default;

    void save(const ChatMessageData& message) override;
    std::vector<ChatMessageData> findByRoomCode(const std::string& roomCode, size_t limit = 50) override;
    bool hasHistoryForCode(const std::string& roomCode) override;
    void deleteByRoomCode(const std::string& roomCode) override;
    void deleteOldestRoomHistory() override;
};

} // namespace infrastructure::adapters::out::persistence

#endif /* !MONGODBCHATMESSAGEREPOSITORY_HPP_ */
