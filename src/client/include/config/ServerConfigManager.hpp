/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ServerConfigManager - Manages server connection configuration
*/

#ifndef SERVERCONFIGMANAGER_HPP_
#define SERVERCONFIGMANAGER_HPP_

#include <string>
#include <cstdint>
#include <filesystem>
#include <mutex>

namespace config {

struct ServerConfig {
    std::string host = "127.0.0.1";
    uint16_t tcpPort = 4125;
    uint16_t udpPort = 4124;
    uint16_t voicePort = 4126;
};

class ServerConfigManager {
public:
    static ServerConfigManager& getInstance();

    // Delete copy/move operations
    ServerConfigManager(const ServerConfigManager&) = delete;
    ServerConfigManager& operator=(const ServerConfigManager&) = delete;
    ServerConfigManager(ServerConfigManager&&) = delete;
    ServerConfigManager& operator=(ServerConfigManager&&) = delete;

    // Configuration access (thread-safe copies)
    ServerConfig getConfig() const;
    std::string getHost() const;
    uint16_t getTcpPort() const;
    uint16_t getUdpPort() const;
    uint16_t getVoicePort() const;

    // Configuration modification
    void setHost(const std::string& host);
    void setTcpPort(uint16_t port);
    void setUdpPort(uint16_t port);
    void setVoicePort(uint16_t port);
    void setConfig(const ServerConfig& config);

    // Quick presets (sets host + ports and saves)
    void applyFrancePreset();
    void applyLocalPreset();

    // Persistence
    bool load();
    bool save() const;
    void resetToDefaults();

    // Get config file path
    std::filesystem::path getConfigFilePath() const;

private:
    ServerConfigManager();
    ~ServerConfigManager() = default;

    bool parseJson(const std::string& json);
    std::string toJson() const;
    std::filesystem::path getConfigDirectory() const;

    ServerConfig _config;
    mutable std::mutex _mutex;

    static constexpr const char* CONFIG_FILENAME = "rtype_client.json";
};

} // namespace config

#endif /* !SERVERCONFIGMANAGER_HPP_ */
