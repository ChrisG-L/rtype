/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** SessionManager implementation
*/

#include "infrastructure/session/SessionManager.hpp"
#include <algorithm>

namespace infrastructure::session {

SessionToken SessionManager::generateToken() {
    SessionToken token;
    // Generate 32 bytes of random data using mt19937_64
    // Each call to _rng() produces 8 bytes
    for (size_t i = 0; i < TOKEN_SIZE; i += 8) {
        uint64_t val = _rng();
        size_t remaining = std::min(static_cast<size_t>(8), TOKEN_SIZE - i);
        std::memcpy(token.bytes + i, &val, remaining);
    }
    return token;
}

std::optional<SessionManager::CreateSessionResult> SessionManager::createSession(
    const std::string& email,
    const std::string& displayName)
{
    std::lock_guard<std::mutex> lock(_mutex);

    // Check if user already has an active session
    auto it = _sessionsByEmail.find(email);
    if (it != _sessionsByEmail.end()) {
        // Session exists - check if it's still valid
        auto& existing = it->second;
        if (existing.status != Session::Status::Expired) {
            return std::nullopt;  // Already has active session
        }
        // Expired session - clean it up
        _tokenToEmail.erase(existing.token.toHex());
        if (!existing.udpEndpoint.empty()) {
            _endpointToEmail.erase(existing.udpEndpoint);
        }
        _sessionsByEmail.erase(it);
    }

    // Generate a new token
    SessionToken token = generateToken();
    std::string tokenHex = token.toHex();

    // Ensure token is unique (extremely unlikely to collide, but be safe)
    while (_tokenToEmail.find(tokenHex) != _tokenToEmail.end()) {
        token = generateToken();
        tokenHex = token.toHex();
    }

    // Create the session
    Session session;
    session.email = email;
    session.displayName = displayName;
    session.token = token;
    session.createdAt = std::chrono::steady_clock::now();
    session.lastActivity = session.createdAt;
    session.status = Session::Status::Pending;

    // Store in indexes
    _sessionsByEmail[email] = session;
    _tokenToEmail[tokenHex] = email;

    return CreateSessionResult{
        .token = token,
        .displayName = displayName
    };
}

std::optional<SessionManager::ValidateResult> SessionManager::validateAndBindUDP(
    const SessionToken& token,
    const std::string& endpoint)
{
    std::lock_guard<std::mutex> lock(_mutex);

    std::string tokenHex = token.toHex();

    // Find session by token
    auto tokenIt = _tokenToEmail.find(tokenHex);
    if (tokenIt == _tokenToEmail.end()) {
        return std::nullopt;  // Token not found
    }

    const std::string& email = tokenIt->second;
    auto sessionIt = _sessionsByEmail.find(email);
    if (sessionIt == _sessionsByEmail.end()) {
        // Inconsistent state - clean up
        _tokenToEmail.erase(tokenIt);
        return std::nullopt;
    }

    Session& session = sessionIt->second;

    // Check if token has expired (only valid for TOKEN_VALIDITY after creation)
    auto now = std::chrono::steady_clock::now();
    if (session.status == Session::Status::Pending) {
        auto elapsed = now - session.createdAt;
        if (elapsed > TOKEN_VALIDITY) {
            session.status = Session::Status::Expired;
            return std::nullopt;
        }
    }

    // Check if already bound to a different endpoint
    if (session.udpBound && session.udpEndpoint != endpoint) {
        return std::nullopt;  // Already connected from different endpoint
    }

    // Bind the endpoint
    session.udpBound = true;
    session.udpEndpoint = endpoint;
    session.lastActivity = now;
    session.status = Session::Status::Active;

    // Update endpoint index
    _endpointToEmail[endpoint] = email;

    return ValidateResult{
        .email = session.email,
        .displayName = session.displayName
    };
}

std::optional<Session> SessionManager::getSessionByEndpoint(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto epIt = _endpointToEmail.find(endpoint);
    if (epIt == _endpointToEmail.end()) {
        return std::nullopt;
    }

    auto sessionIt = _sessionsByEmail.find(epIt->second);
    if (sessionIt == _sessionsByEmail.end()) {
        return std::nullopt;
    }

    return sessionIt->second;
}

void SessionManager::assignPlayerId(const std::string& endpoint, uint8_t playerId) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto epIt = _endpointToEmail.find(endpoint);
    if (epIt == _endpointToEmail.end()) {
        return;
    }

    auto sessionIt = _sessionsByEmail.find(epIt->second);
    if (sessionIt != _sessionsByEmail.end()) {
        sessionIt->second.playerId = playerId;
    }
}

std::optional<uint8_t> SessionManager::getPlayerIdByEndpoint(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto epIt = _endpointToEmail.find(endpoint);
    if (epIt == _endpointToEmail.end()) {
        return std::nullopt;
    }

    auto sessionIt = _sessionsByEmail.find(epIt->second);
    if (sessionIt == _sessionsByEmail.end()) {
        return std::nullopt;
    }

    return sessionIt->second.playerId;
}

void SessionManager::updateActivity(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto epIt = _endpointToEmail.find(endpoint);
    if (epIt == _endpointToEmail.end()) {
        return;
    }

    auto sessionIt = _sessionsByEmail.find(epIt->second);
    if (sessionIt != _sessionsByEmail.end()) {
        sessionIt->second.lastActivity = std::chrono::steady_clock::now();
    }
}

void SessionManager::removeSession(const std::string& email) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto sessionIt = _sessionsByEmail.find(email);
    if (sessionIt == _sessionsByEmail.end()) {
        return;
    }

    // Clean up indexes
    _tokenToEmail.erase(sessionIt->second.token.toHex());
    if (!sessionIt->second.udpEndpoint.empty()) {
        _endpointToEmail.erase(sessionIt->second.udpEndpoint);
    }

    _sessionsByEmail.erase(sessionIt);
}

void SessionManager::removeSessionByEndpoint(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto epIt = _endpointToEmail.find(endpoint);
    if (epIt == _endpointToEmail.end()) {
        return;
    }

    std::string email = epIt->second;
    _endpointToEmail.erase(epIt);

    auto sessionIt = _sessionsByEmail.find(email);
    if (sessionIt != _sessionsByEmail.end()) {
        _tokenToEmail.erase(sessionIt->second.token.toHex());
        _sessionsByEmail.erase(sessionIt);
    }
}

std::vector<uint8_t> SessionManager::cleanupExpiredSessions() {
    std::lock_guard<std::mutex> lock(_mutex);

    std::vector<uint8_t> expiredPlayerIds;
    auto now = std::chrono::steady_clock::now();

    std::vector<std::string> toRemove;

    for (auto& [email, session] : _sessionsByEmail) {
        bool expired = false;

        if (session.status == Session::Status::Pending) {
            // Pending sessions expire after TOKEN_VALIDITY
            if (now - session.createdAt > TOKEN_VALIDITY) {
                expired = true;
            }
        } else if (session.status == Session::Status::Active) {
            // Active sessions expire after SESSION_TIMEOUT of inactivity
            if (now - session.lastActivity > SESSION_TIMEOUT) {
                expired = true;
            }
        }

        if (expired) {
            session.status = Session::Status::Expired;
            if (session.playerId.has_value()) {
                expiredPlayerIds.push_back(*session.playerId);
            }
            toRemove.push_back(email);
        }
    }

    // Remove expired sessions
    for (const auto& email : toRemove) {
        auto sessionIt = _sessionsByEmail.find(email);
        if (sessionIt != _sessionsByEmail.end()) {
            _tokenToEmail.erase(sessionIt->second.token.toHex());
            if (!sessionIt->second.udpEndpoint.empty()) {
                _endpointToEmail.erase(sessionIt->second.udpEndpoint);
            }
            _sessionsByEmail.erase(sessionIt);
        }
    }

    return expiredPlayerIds;
}

bool SessionManager::hasActiveSession(const std::string& email) const {
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _sessionsByEmail.find(email);
    if (it == _sessionsByEmail.end()) {
        return false;
    }

    return it->second.status != Session::Status::Expired;
}

std::vector<std::string> SessionManager::getAllActiveEndpoints() const {
    std::lock_guard<std::mutex> lock(_mutex);

    std::vector<std::string> endpoints;
    for (const auto& [endpoint, email] : _endpointToEmail) {
        auto sessionIt = _sessionsByEmail.find(email);
        if (sessionIt != _sessionsByEmail.end() &&
            sessionIt->second.status == Session::Status::Active) {
            endpoints.push_back(endpoint);
        }
    }
    return endpoints;
}

std::vector<Session> SessionManager::getAllSessions() const {
    std::lock_guard<std::mutex> lock(_mutex);

    std::vector<Session> sessions;
    sessions.reserve(_sessionsByEmail.size());
    for (const auto& [email, session] : _sessionsByEmail) {
        sessions.push_back(session);
    }
    return sessions;
}

size_t SessionManager::getSessionCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _sessionsByEmail.size();
}

void SessionManager::banUser(const std::string& email) {
    std::lock_guard<std::mutex> lock(_mutex);

    // Capture display name from session if available
    std::string displayName;
    auto sessionIt = _sessionsByEmail.find(email);
    if (sessionIt != _sessionsByEmail.end()) {
        displayName = sessionIt->second.displayName;

        // Remove the active session
        _tokenToEmail.erase(sessionIt->second.token.toHex());
        if (!sessionIt->second.udpEndpoint.empty()) {
            _endpointToEmail.erase(sessionIt->second.udpEndpoint);
        }
        _sessionsByEmail.erase(sessionIt);
    }

    // Add to banned map
    _bannedUsers[email] = BannedUser{
        .email = email,
        .displayName = displayName
    };
}

void SessionManager::unbanUser(const std::string& email) {
    std::lock_guard<std::mutex> lock(_mutex);
    _bannedUsers.erase(email);
}

bool SessionManager::isBanned(const std::string& email) const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _bannedUsers.contains(email);
}

std::vector<BannedUser> SessionManager::getBannedUsers() const {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<BannedUser> result;
    result.reserve(_bannedUsers.size());
    for (const auto& [email, bannedUser] : _bannedUsers) {
        result.push_back(bannedUser);
    }
    return result;
}

} // namespace infrastructure::session
