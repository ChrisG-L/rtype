/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** SessionManager - Manages authenticated sessions between TCP and UDP
*/

#ifndef SESSIONMANAGER_HPP_
#define SESSIONMANAGER_HPP_

#include <string>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <random>
#include "Protocol.hpp"

namespace infrastructure::session {

struct Session {
    std::string email;              // Unique identifier (from TCP login)
    std::string displayName;        // Display name for the player
    SessionToken token;             // Session token for UDP auth
    std::chrono::steady_clock::time_point createdAt;
    std::chrono::steady_clock::time_point lastActivity;

    // UDP binding (set when client sends JoinGame)
    bool udpBound = false;
    std::string udpEndpoint;        // "ip:port" string
    std::optional<uint8_t> playerId;

    // Session state
    enum class Status { Pending, Active, Expired };
    Status status = Status::Pending;
};

struct BannedUser {
    std::string email;
    std::string displayName;  // Captured at ban time (may be empty if unknown)
};

class SessionManager {
public:
    // Token validity before UDP connection (5 minutes)
    static constexpr auto TOKEN_VALIDITY = std::chrono::minutes(5);

    // Session timeout for inactivity (30 seconds)
    static constexpr auto SESSION_TIMEOUT = std::chrono::seconds(30);

    SessionManager() = default;
    ~SessionManager() = default;

    // ═══════════════════════════════════════════════════════════════════
    // Called by TCPAuthServer after successful login
    // ═══════════════════════════════════════════════════════════════════

    struct CreateSessionResult {
        SessionToken token;
        std::string displayName;
    };

    // Creates a session and returns the token
    // Returns nullopt if the user already has an active session
    std::optional<CreateSessionResult> createSession(const std::string& email,
                                                      const std::string& displayName);

    // ═══════════════════════════════════════════════════════════════════
    // Called by UDPServer to validate connections
    // ═══════════════════════════════════════════════════════════════════

    struct ValidateResult {
        std::string email;
        std::string displayName;
    };

    // Validates a token and binds the UDP endpoint to the session
    // Returns email/displayName if valid, nullopt otherwise
    std::optional<ValidateResult> validateAndBindUDP(const SessionToken& token,
                                                      const std::string& endpoint);

    // Gets session by UDP endpoint (for subsequent messages)
    std::optional<Session> getSessionByEndpoint(const std::string& endpoint);

    // Assigns a playerId to a session (after GameWorld.addPlayer)
    void assignPlayerId(const std::string& endpoint, uint8_t playerId);

    // Gets playerId by endpoint (quick lookup for message handling)
    std::optional<uint8_t> getPlayerIdByEndpoint(const std::string& endpoint);

    // ═══════════════════════════════════════════════════════════════════
    // Lifecycle management
    // ═══════════════════════════════════════════════════════════════════

    // Updates activity timestamp (called on each UDP message received)
    void updateActivity(const std::string& endpoint);

    // Removes a session (voluntary disconnect or timeout)
    void removeSession(const std::string& email);
    void removeSessionByEndpoint(const std::string& endpoint);

    // Cleans up expired sessions, returns playerIds of removed sessions
    std::vector<uint8_t> cleanupExpiredSessions();

    // Checks if a user already has an active session
    bool hasActiveSession(const std::string& email) const;

    // Gets all active endpoints (for broadcasting)
    std::vector<std::string> getAllActiveEndpoints() const;

    // Gets all sessions (for CLI/debugging)
    std::vector<Session> getAllSessions() const;

    // Gets session count
    size_t getSessionCount() const;

    // ═══════════════════════════════════════════════════════════════════
    // Ban management
    // ═══════════════════════════════════════════════════════════════════

    // Ban a user by email (permanent until unban)
    void banUser(const std::string& email);

    // Unban a user by email
    void unbanUser(const std::string& email);

    // Check if a user is banned
    bool isBanned(const std::string& email) const;

    // Get all banned users (with display names)
    std::vector<BannedUser> getBannedUsers() const;

private:
    mutable std::mutex _mutex;

    // Primary index: by email (one user = one session max)
    std::unordered_map<std::string, Session> _sessionsByEmail;

    // Secondary index: token (hex) -> email (for quick token validation)
    std::unordered_map<std::string, std::string> _tokenToEmail;

    // Secondary index: endpoint -> email (for quick endpoint lookup)
    std::unordered_map<std::string, std::string> _endpointToEmail;

    // Banned users (email -> BannedUser)
    std::unordered_map<std::string, BannedUser> _bannedUsers;

    // Generates a cryptographically random token
    SessionToken generateToken();

    // Random number generator for token generation
    std::random_device _rd;
    std::mt19937_64 _rng{_rd()};
};

} // namespace infrastructure::session

#endif /* !SESSIONMANAGER_HPP_ */
