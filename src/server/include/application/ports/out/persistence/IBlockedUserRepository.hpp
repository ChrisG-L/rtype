/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** IBlockedUserRepository - Interface for blocked users persistence
*/

#ifndef IBLOCKEDUSERREPOSITORY_HPP_
#define IBLOCKEDUSERREPOSITORY_HPP_

#include <string>
#include <vector>
#include <chrono>

namespace application::ports::out::persistence {

struct BlockedUserData {
    std::string blockedEmail;
    std::string blockedDisplayName;
    std::chrono::system_clock::time_point createdAt;
};

class IBlockedUserRepository {
public:
    virtual ~IBlockedUserRepository() = default;

    /**
     * Blocks a user
     * @param blockerEmail Who is blocking
     * @param blockedEmail Who is being blocked
     * @param blockedDisplayName Display name of blocked user
     */
    virtual void blockUser(
        const std::string& blockerEmail,
        const std::string& blockedEmail,
        const std::string& blockedDisplayName) = 0;

    /**
     * Unblocks a user
     * @param blockerEmail Who blocked
     * @param blockedEmail Who was blocked
     */
    virtual void unblockUser(
        const std::string& blockerEmail,
        const std::string& blockedEmail) = 0;

    /**
     * Checks if blocker has blocked the blocked user
     * @param blockerEmail Who might have blocked
     * @param blockedEmail Who might be blocked
     * @return true if blocked
     */
    virtual bool isBlocked(
        const std::string& blockerEmail,
        const std::string& blockedEmail) = 0;

    /**
     * Checks if either user has blocked the other
     * @param email1 First user
     * @param email2 Second user
     * @return true if there's any block in either direction
     */
    virtual bool hasAnyBlock(const std::string& email1, const std::string& email2) = 0;

    /**
     * Gets the list of users blocked by a user
     * @param blockerEmail Who blocked
     * @return List of blocked users
     */
    virtual std::vector<BlockedUserData> getBlockedUsers(const std::string& blockerEmail) = 0;
};

} // namespace application::ports::out::persistence

#endif /* !IBLOCKEDUSERREPOSITORY_HPP_ */
