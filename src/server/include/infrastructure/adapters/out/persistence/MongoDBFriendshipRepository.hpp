/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBFriendshipRepository - MongoDB implementation for friendships
*/

#ifndef MONGODBFRIENDSHIPREPOSITORY_HPP_
#define MONGODBFRIENDSHIPREPOSITORY_HPP_

#include "application/ports/out/persistence/IFriendshipRepository.hpp"
#include "MongoDBConfiguration.hpp"
#include <memory>

namespace infrastructure::adapters::out::persistence {

class MongoDBFriendshipRepository : public application::ports::out::persistence::IFriendshipRepository {
public:
    explicit MongoDBFriendshipRepository(std::shared_ptr<MongoDBConfiguration> mongoDB);
    ~MongoDBFriendshipRepository() override = default;

    void addFriendship(const std::string& email1, const std::string& email2) override;
    void removeFriendship(const std::string& email1, const std::string& email2) override;
    bool areFriends(const std::string& email1, const std::string& email2) override;
    std::vector<std::string> getFriendEmails(
        const std::string& email,
        size_t offset = 0,
        size_t limit = 50) override;
    size_t getFriendCount(const std::string& email) override;

private:
    std::shared_ptr<MongoDBConfiguration> _mongoDB;
    static constexpr const char* COLLECTION_NAME = "friendships";

    // Helper to ensure consistent ordering (alphabetical)
    static std::pair<std::string, std::string> orderEmails(
        const std::string& email1, const std::string& email2);
};

} // namespace infrastructure::adapters::out::persistence

#endif /* !MONGODBFRIENDSHIPREPOSITORY_HPP_ */
