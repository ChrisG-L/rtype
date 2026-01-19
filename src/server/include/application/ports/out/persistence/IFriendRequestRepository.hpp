/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** IFriendRequestRepository - Interface for friend request persistence
*/

#ifndef IFRIENDREQUESTREPOSITORY_HPP_
#define IFRIENDREQUESTREPOSITORY_HPP_

#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace application::ports::out::persistence {

struct FriendRequestData {
    std::string fromEmail;
    std::string toEmail;
    std::string fromDisplayName;
    std::chrono::system_clock::time_point createdAt;
};

class IFriendRequestRepository {
public:
    virtual ~IFriendRequestRepository() = default;

    /**
     * Creates a friend request
     * @param fromEmail Sender's email
     * @param toEmail Recipient's email
     * @param fromDisplayName Sender's display name
     */
    virtual void createRequest(
        const std::string& fromEmail,
        const std::string& toEmail,
        const std::string& fromDisplayName) = 0;

    /**
     * Deletes a friend request
     * @param fromEmail Original sender's email
     * @param toEmail Original recipient's email
     */
    virtual void deleteRequest(const std::string& fromEmail, const std::string& toEmail) = 0;

    /**
     * Checks if a request exists from one user to another
     * @param fromEmail Sender's email
     * @param toEmail Recipient's email
     * @return true if request exists
     */
    virtual bool requestExists(const std::string& fromEmail, const std::string& toEmail) = 0;

    /**
     * Gets a specific friend request
     * @param fromEmail Sender's email
     * @param toEmail Recipient's email
     * @return Request data or nullopt
     */
    virtual std::optional<FriendRequestData> getRequest(
        const std::string& fromEmail,
        const std::string& toEmail) = 0;

    /**
     * Gets all incoming requests for a user
     * @param email Recipient's email
     * @return List of incoming requests
     */
    virtual std::vector<FriendRequestData> getIncomingRequests(const std::string& email) = 0;

    /**
     * Gets all outgoing requests from a user
     * @param email Sender's email
     * @return List of outgoing requests
     */
    virtual std::vector<FriendRequestData> getOutgoingRequests(const std::string& email) = 0;

    /**
     * Counts pending incoming requests for a user
     * @param email Recipient's email
     * @return Number of pending requests
     */
    virtual size_t getPendingRequestCount(const std::string& email) = 0;
};

} // namespace application::ports::out::persistence

#endif /* !IFRIENDREQUESTREPOSITORY_HPP_ */
