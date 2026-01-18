/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** VersionHistoryManager - Loads and manages git hash history for version tracking
*/

#ifndef VERSIONHISTORYMANAGER_HPP_
#define VERSIONHISTORYMANAGER_HPP_

#include "Protocol.hpp"
#include <string>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <cstdio>
#include <spdlog/spdlog.h>

namespace infrastructure::version {

/**
 * @brief Manages version history for tracking how many commits behind clients are
 *
 * Loads version history from a file (version_history.txt) that contains
 * recent git hashes, one per line (newest first).
 *
 * The file should be generated during deployment using:
 *   git log --format="%h" -n 50 > version_history.txt
 */
class VersionHistoryManager {
public:
    static VersionHistoryManager& getInstance() {
        static VersionHistoryManager instance;
        return instance;
    }

    /**
     * @brief Load version history from file
     * @param filepath Path to version_history.txt (default: "version_history.txt")
     * @return true if loaded successfully
     */
    bool loadFromFile(const std::string& filepath = "version_history.txt") {
        std::scoped_lock lock(_mutex);

        // Initialize with current version as first entry
        _history.count = 0;
        std::memset(_history.hashes, 0, sizeof(_history.hashes));

        // Always add current server hash as first entry
        std::snprintf(_history.hashes[0], GIT_HASH_LEN, "%s", RTYPE_GIT_HASH);
        _history.count = 1;

        if (!std::filesystem::exists(filepath)) {
            spdlog::warn("Version history file not found: {} - using current hash only", filepath);
            return false;
        }

        std::ifstream file(filepath);
        if (!file.is_open()) {
            spdlog::error("Failed to open version history file: {}", filepath);
            return false;
        }

        std::string line;
        // Skip first line if it matches current hash (avoid duplicate)
        if (std::getline(file, line)) {
            // Trim whitespace
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' ')) {
                line.pop_back();
            }
            // If not matching current, add it
            if (line.length() >= 8 && std::strncmp(line.c_str(), RTYPE_GIT_HASH, 8) != 0) {
                std::snprintf(_history.hashes[_history.count], GIT_HASH_LEN, "%s", line.c_str());
                _history.count++;
            }
        }

        // Read remaining entries
        while (std::getline(file, line) && _history.count < MAX_VERSION_HISTORY) {
            // Trim whitespace
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' ')) {
                line.pop_back();
            }
            if (line.length() >= 8) {  // Git short hash is 8 chars
                std::snprintf(_history.hashes[_history.count], GIT_HASH_LEN, "%s", line.c_str());
                _history.count++;
            }
        }

        spdlog::info("Loaded {} version history entries from {}", _history.count, filepath);
        return true;
    }

    /**
     * @brief Get the version history
     * @return VersionHistory structure
     */
    const VersionHistory& getHistory() const {
        return _history;
    }

    /**
     * @brief Find how many commits behind a client hash is
     * @param clientHash The client's git hash
     * @return Number of commits behind (0 = up to date, -1 = not found/too old)
     */
    int getCommitsBehind(const char* clientHash) const {
        return _history.findPosition(clientHash);
    }

private:
    VersionHistoryManager() {
        // Initialize with just the current hash
        _history.count = 1;
        std::memset(_history.hashes, 0, sizeof(_history.hashes));
        std::snprintf(_history.hashes[0], GIT_HASH_LEN, "%s", RTYPE_GIT_HASH);
    }

    VersionHistory _history;
    mutable std::mutex _mutex;
};

} // namespace infrastructure::version

#endif /* !VERSIONHISTORYMANAGER_HPP_ */
