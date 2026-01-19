/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBPrivateMessageRepository - MongoDB implementation for private messages
*/

#ifndef MONGODBPRIVATEMESSAGEREPOSITORY_HPP_
#define MONGODBPRIVATEMESSAGEREPOSITORY_HPP_

#include "application/ports/out/persistence/IPrivateMessageRepository.hpp"
#include "MongoDBConfiguration.hpp"
#include <memory>

namespace infrastructure::adapters::out::persistence {

class MongoDBPrivateMessageRepository : public application::ports::out::persistence::IPrivateMessageRepository {
public:
    explicit MongoDBPrivateMessageRepository(std::shared_ptr<MongoDBConfiguration> mongoDB);
    ~MongoDBPrivateMessageRepository() override = default;

    uint64_t saveMessage(
        const std::string& senderEmail,
        const std::string& recipientEmail,
        const std::string& senderDisplayName,
        const std::string& message) override;

    std::vector<application::ports::out::persistence::PrivateMessageData> getConversation(
        const std::string& email1,
        const std::string& email2,
        size_t offset = 0,
        size_t limit = 50) override;

    std::vector<application::ports::out::persistence::ConversationSummaryData> getConversationsList(
        const std::string& email,
        size_t limit = 50) override;

    void markAsRead(
        const std::string& readerEmail,
        const std::string& senderEmail) override;

    size_t getUnreadCount(const std::string& email) override;

    size_t getUnreadCountFrom(
        const std::string& recipientEmail,
        const std::string& senderEmail) override;

    // ============ Admin functions ============
    std::vector<application::ports::out::persistence::PrivateMessageData> getMessagesByUser(
        const std::string& email, size_t limit = 100) override;

    std::vector<application::ports::out::persistence::PrivateMessageData> getAllMessages(
        size_t limit = 100, uint64_t beforeTimestamp = 0) override;

    std::vector<application::ports::out::persistence::PrivateMessageData> searchMessages(
        const std::string& searchTerm, size_t limit = 50) override;

    std::pair<size_t, size_t> getMessageStats() override;

private:
    std::shared_ptr<MongoDBConfiguration> _mongoDB;
    static constexpr const char* COLLECTION_NAME = "private_messages";

    // Generate conversation key (alphabetically ordered)
    static std::string makeConversationKey(const std::string& email1, const std::string& email2);

    application::ports::out::persistence::PrivateMessageData documentToMessage(
        const bsoncxx::document::view& doc);
};

} // namespace infrastructure::adapters::out::persistence

#endif /* !MONGODBPRIVATEMESSAGEREPOSITORY_HPP_ */
