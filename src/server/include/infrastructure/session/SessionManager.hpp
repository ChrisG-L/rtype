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
#include <functional>
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

    // Room configuration (set when game starts)
    uint16_t roomGameSpeedPercent = 100;  // 50-200, default 100%
    std::string roomCode;           // Room code for multi-instance routing

    // Hidden GodMode (loaded from DB, toggled via /toggleGodMode in chat)
    bool godMode = false;

    // Session state
    enum class Status { Pending, Active, Expired };
    Status status = Status::Pending;
};

struct BannedUser {
    std::string email;
    std::string displayName;  // Captured at ban time (may be empty if unknown)
};

// Callback type for kick notifications (used by TCP sessions)
using KickedCallback = std::function<void(const std::string& reason)>;

// Callback type for when a player leaves the game (used by UDPServer to clean up GameWorld)
// Parameters: playerId, roomCode, udpEndpoint
using PlayerLeaveGameCallback = std::function<void(uint8_t playerId, const std::string& roomCode, const std::string& endpoint)>;

// Callback type for when a player's GodMode changes (used by UDPServer to update GameWorld)
// Parameters: playerId, roomCode, enabled
using GodModeChangedCallback = std::function<void(uint8_t playerId, const std::string& roomCode, bool enabled)>;

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
        uint8_t playerId;  // 0 if not yet assigned
    };

    // Validates a token without binding (for voice server which uses separate endpoint)
    // Returns email/displayName/playerId if valid, nullopt otherwise
    std::optional<ValidateResult> validateToken(const SessionToken& token);

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

    // Sets room game speed for a session (called when game starts)
    void setRoomGameSpeed(const std::string& email, uint16_t gameSpeedPercent);

    // Gets room game speed by endpoint (for JoinGame handling)
    uint16_t getRoomGameSpeedByEndpoint(const std::string& endpoint) const;

    // Sets room code for a session (called when game starts)
    void setRoomCode(const std::string& email, const std::string& roomCode);

    // Gets room code by endpoint (for routing UDP messages to correct GameWorld)
    std::optional<std::string> getRoomCodeByEndpoint(const std::string& endpoint) const;

    // ═══════════════════════════════════════════════════════════════════
    // GodMode management (hidden feature)
    // ═══════════════════════════════════════════════════════════════════

    // Sets GodMode for a session (called when loading from DB or toggling)
    void setGodMode(const std::string& email, bool enabled);

    // Toggles GodMode for a session (returns new state)
    bool toggleGodMode(const std::string& email);

    // Checks if a player is in GodMode by playerId
    bool isPlayerInGodMode(uint8_t playerId) const;

    // Checks if a player is in GodMode by email
    bool isGodModeEnabled(const std::string& email) const;

    // Gets email by playerId (for kick system - reverse lookup)
    std::optional<std::string> getEmailByPlayerId(uint8_t playerId) const;

    // Gets session by email (returns copy)
    std::optional<Session> getSessionByEmail(const std::string& email) const;

    // ═══════════════════════════════════════════════════════════════════
    // Lifecycle management
    // ═══════════════════════════════════════════════════════════════════

    // Updates activity timestamp (called on each UDP message received)
    void updateActivity(const std::string& endpoint);

    // Removes a session (voluntary disconnect or timeout)
    void removeSession(const std::string& email);
    void removeSessionByEndpoint(const std::string& endpoint);

    // Clears UDP binding but keeps TCP session active (for kick from game)
    // This allows the player to rejoin a new room after being kicked
    void clearUDPBinding(const std::string& endpoint);

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

    // ═══════════════════════════════════════════════════════════════════
    // In-game kick system (for kicking players during gameplay via TCP)
    // ═══════════════════════════════════════════════════════════════════

    // Register a callback for when this player is kicked (called by TCP Session)
    void registerKickedCallback(const std::string& email, KickedCallback callback);

    // Unregister the kicked callback (called when TCP Session closes)
    void unregisterKickedCallback(const std::string& email);

    // Kick a player by email - calls the registered callback with reason
    // Returns true if the player was found and notified, false otherwise
    bool kickPlayerByEmail(const std::string& email, const std::string& reason);

    // ═══════════════════════════════════════════════════════════════════
    // Player leave game notification (for UDPServer cleanup)
    // ═══════════════════════════════════════════════════════════════════

    // Register callback for when a player leaves the game (called by UDPServer)
    void setPlayerLeaveGameCallback(PlayerLeaveGameCallback callback);

    // Notify that a player is leaving the game (clears UDP binding and calls callback)
    // Called by TCPAuthServer when player leaves room during active game
    void notifyPlayerLeaveGame(const std::string& email);

    // ═══════════════════════════════════════════════════════════════════
    // GodMode notification (for real-time sync with GameWorld)
    // ═══════════════════════════════════════════════════════════════════

    // Register callback for when a player's GodMode changes (called by UDPServer)
    void setGodModeChangedCallback(GodModeChangedCallback callback);

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

    // Kick callbacks (email -> callback) for in-game kick notifications
    std::unordered_map<std::string, KickedCallback> _kickedCallbacks;

    // Callback for player leaving game (called to notify UDPServer)
    PlayerLeaveGameCallback _playerLeaveGameCallback;

    // Callback for GodMode changes (called to notify UDPServer)
    GodModeChangedCallback _godModeChangedCallback;

    // Generates a cryptographically secure random token using OpenSSL RAND_bytes
    SessionToken generateToken();
};

} // namespace infrastructure::session

#endif /* !SESSIONMANAGER_HPP_ */
