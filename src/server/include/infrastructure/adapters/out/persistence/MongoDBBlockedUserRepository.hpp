/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBBlockedUserRepository - MongoDB implementation for blocked users
*/

#ifndef MONGODBBLOCKEDUSERREPOSITORY_HPP_
#define MONGODBBLOCKEDUSERREPOSITORY_HPP_

#include "application/ports/out/persistence/IBlockedUserRepository.hpp"
#include "MongoDBConfiguration.hpp"
#include <memory>

namespace infrastructure::adapters::out::persistence {

class MongoDBBlockedUserRepository : public application::ports::out::persistence::IBlockedUserRepository {
public:
    explicit MongoDBBlockedUserRepository(std::shared_ptr<MongoDBConfiguration> mongoDB);
    ~MongoDBBlockedUserRepository() override = default;

    void blockUser(
        const std::string& blockerEmail,
        const std::string& blockedEmail,
        const std::string& blockedDisplayName) override;

    void unblockUser(
        const std::string& blockerEmail,
        const std::string& blockedEmail) override;

    bool isBlocked(
        const std::string& blockerEmail,
        const std::string& blockedEmail) override;

    bool hasAnyBlock(const std::string& email1, const std::string& email2) override;

    std::vector<application::ports::out::persistence::BlockedUserData> getBlockedUsers(
        const std::string& blockerEmail) override;

private:
    std::shared_ptr<MongoDBConfiguration> _mongoDB;
    static constexpr const char* COLLECTION_NAME = "blocked_users";

    application::ports::out::persistence::BlockedUserData documentToBlockedUser(
        const bsoncxx::document::view& doc);
};

} // namespace infrastructure::adapters::out::persistence

#endif /* !MONGODBBLOCKEDUSERREPOSITORY_HPP_ */
