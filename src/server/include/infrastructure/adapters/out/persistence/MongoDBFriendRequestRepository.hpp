/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBFriendRequestRepository - MongoDB implementation for friend requests
*/

#ifndef MONGODBFRIENDREQUESTREPOSITORY_HPP_
#define MONGODBFRIENDREQUESTREPOSITORY_HPP_

#include "application/ports/out/persistence/IFriendRequestRepository.hpp"
#include "MongoDBConfiguration.hpp"
#include <memory>

namespace infrastructure::adapters::out::persistence {

class MongoDBFriendRequestRepository : public application::ports::out::persistence::IFriendRequestRepository {
public:
    explicit MongoDBFriendRequestRepository(std::shared_ptr<MongoDBConfiguration> mongoDB);
    ~MongoDBFriendRequestRepository() override = default;

    void createRequest(
        const std::string& fromEmail,
        const std::string& toEmail,
        const std::string& fromDisplayName) override;

    void deleteRequest(const std::string& fromEmail, const std::string& toEmail) override;

    bool requestExists(const std::string& fromEmail, const std::string& toEmail) override;

    std::optional<application::ports::out::persistence::FriendRequestData> getRequest(
        const std::string& fromEmail,
        const std::string& toEmail) override;

    std::vector<application::ports::out::persistence::FriendRequestData> getIncomingRequests(
        const std::string& email) override;

    std::vector<application::ports::out::persistence::FriendRequestData> getOutgoingRequests(
        const std::string& email) override;

    size_t getPendingRequestCount(const std::string& email) override;

private:
    std::shared_ptr<MongoDBConfiguration> _mongoDB;
    static constexpr const char* COLLECTION_NAME = "friend_requests";

    application::ports::out::persistence::FriendRequestData documentToRequest(
        const bsoncxx::document::view& doc);
};

} // namespace infrastructure::adapters::out::persistence

#endif /* !MONGODBFRIENDREQUESTREPOSITORY_HPP_ */
