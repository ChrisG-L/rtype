/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Version - Client version info and dev mode detection
*/

#ifndef VERSION_HPP_
#define VERSION_HPP_

#include "Protocol.hpp"
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <cstdio>

namespace client::core {

// Check if running in dev mode (version.dev file exists)
inline bool isDevMode() {
    // Check multiple locations for version.dev:
    // 1. Current working directory
    // 2. Next to executable (if we can determine it)
    // 3. Project root (for development)

    const std::array<std::string, 4> paths = {
        "version.dev",
        "./version.dev",
        "../version.dev",
        "../../version.dev"  // When running from build directory
    };

    for (const auto& path : paths) {
        if (std::filesystem::exists(path)) {
            return true;
        }
    }

    return false;
}

// Get the client version info
inline VersionInfo getClientVersion() {
    VersionInfo version;
    version.major = RTYPE_VERSION_MAJOR;
    version.minor = RTYPE_VERSION_MINOR;
    version.patch = RTYPE_VERSION_PATCH;
    version.flags = 0;

    // Allow overriding git hash via environment variable for testing
    // Usage: RTYPE_TEST_HASH=abc1234 ./rtype_client
    const char* testHash = std::getenv("RTYPE_TEST_HASH");
    if (testHash != nullptr && std::strlen(testHash) > 0) {
        std::snprintf(version.gitHash, GIT_HASH_LEN, "%s", testHash);
    } else {
        std::snprintf(version.gitHash, GIT_HASH_LEN, "%s", RTYPE_GIT_HASH);
    }

    // Check for dev mode (version.dev file exists next to executable)
    if (isDevMode()) {
        version.setDev(true);
    }

    return version;
}

// Format version for display (e.g., "1.0.0 (abc12345)")
inline std::string formatVersion(const VersionInfo& version) {
    std::string result = std::to_string(version.major) + "." +
                         std::to_string(version.minor) + "." +
                         std::to_string(version.patch);
    if (version.gitHash[0] != '\0' && std::strcmp(version.gitHash, "unknown") != 0) {
        result += " (" + std::string(version.gitHash) + ")";
    }
    if (version.isDev()) {
        result += " [DEV]";
    }
    return result;
}

// Check if client version is compatible with server version
// Returns true if compatible, false if update needed
inline bool isVersionCompatible(const VersionInfo& client, const VersionInfo& server) {
    // If client is in dev mode, always compatible (bypass version check)
    if (client.isDev()) {
        return true;
    }

    // Check if git hashes match (exact version check)
    return client.isExactMatch(server);
}

} // namespace client::core

#endif /* !VERSION_HPP_ */
