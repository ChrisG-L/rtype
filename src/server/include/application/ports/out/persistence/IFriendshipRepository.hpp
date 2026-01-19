/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** IFriendshipRepository - Interface for friendship persistence
*/

#ifndef IFRIENDSHIPREPOSITORY_HPP_
#define IFRIENDSHIPREPOSITORY_HPP_

#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace application::ports::out::persistence {

struct FriendshipData {
    std::string friendEmail;
    std::string friendDisplayName;
    std::chrono::system_clock::time_point createdAt;
};

class IFriendshipRepository {
public:
    virtual ~IFriendshipRepository() = default;

    /**
     * Adds a friendship relationship (bidirectional)
     * @param email1 First user email
     * @param email2 Second user email
     */
    virtual void addFriendship(const std::string& email1, const std::string& email2) = 0;

    /**
     * Removes a friendship relationship
     * @param email1 First user email
     * @param email2 Second user email
     */
    virtual void removeFriendship(const std::string& email1, const std::string& email2) = 0;

    /**
     * Checks if two users are friends
     * @param email1 First user email
     * @param email2 Second user email
     * @return true if they are friends
     */
    virtual bool areFriends(const std::string& email1, const std::string& email2) = 0;

    /**
     * Gets the list of friend emails for a user
     * @param email User's email
     * @param offset Pagination offset
     * @param limit Maximum results (default 50)
     * @return List of friend emails
     */
    virtual std::vector<std::string> getFriendEmails(
        const std::string& email,
        size_t offset = 0,
        size_t limit = 50) = 0;

    /**
     * Counts the number of friends for a user
     * @param email User's email
     * @return Number of friends
     */
    virtual size_t getFriendCount(const std::string& email) = 0;
};

} // namespace application::ports::out::persistence

#endif /* !IFRIENDSHIPREPOSITORY_HPP_ */
