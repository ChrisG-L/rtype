/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ServerConfigManager - Manages server connection configuration
*/

#include "config/ServerConfigManager.hpp"
#include "core/Logger.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <regex>

namespace config {

ServerConfigManager& ServerConfigManager::getInstance()
{
    static ServerConfigManager instance;
    return instance;
}

ServerConfigManager::ServerConfigManager()
{
    // Try to load config from file, use defaults if not found
    if (!load()) {
        auto logger = client::logging::Logger::getBootLogger();
        if (logger) {
            logger->info("No config file found, using defaults ({}:{})", _config.host, _config.tcpPort);
        }
    }
}

ServerConfig ServerConfigManager::getConfig() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _config;
}

std::string ServerConfigManager::getHost() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _config.host;
}

uint16_t ServerConfigManager::getTcpPort() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _config.tcpPort;
}

uint16_t ServerConfigManager::getUdpPort() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _config.udpPort;
}

uint16_t ServerConfigManager::getVoicePort() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _config.voicePort;
}

void ServerConfigManager::setHost(const std::string& host)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _config.host = host;
}

void ServerConfigManager::setTcpPort(uint16_t port)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _config.tcpPort = port;
}

void ServerConfigManager::setUdpPort(uint16_t port)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _config.udpPort = port;
}

void ServerConfigManager::setVoicePort(uint16_t port)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _config.voicePort = port;
}

void ServerConfigManager::setConfig(const ServerConfig& config)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _config = config;
}

void ServerConfigManager::applyFrancePreset()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _config.host = "51.254.137.175";
        _config.tcpPort = 4125;
        _config.udpPort = 4124;
    }
    save();
}

void ServerConfigManager::applyLocalPreset()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _config.host = "127.0.0.1";
        _config.tcpPort = 4125;
        _config.udpPort = 4124;
    }
    save();
}

void ServerConfigManager::resetToDefaults()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _config = ServerConfig{};
}

std::filesystem::path ServerConfigManager::getConfigDirectory() const
{
    // Cross-platform config directory
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        return std::filesystem::path(appdata) / "RType";
    }
    return std::filesystem::path(".") / ".config";
#else
    const char* home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home) / ".config" / "rtype";
    }
    return std::filesystem::path(".") / ".config";
#endif
}

std::filesystem::path ServerConfigManager::getConfigFilePath() const
{
    return getConfigDirectory() / CONFIG_FILENAME;
}

bool ServerConfigManager::load()
{
    auto configPath = getConfigFilePath();

    if (!std::filesystem::exists(configPath)) {
        return false;
    }

    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return parseJson(buffer.str());
}

bool ServerConfigManager::save() const
{
    auto configDir = getConfigDirectory();

    // Create config directory if it doesn't exist
    std::error_code ec;
    std::filesystem::create_directories(configDir, ec);
    if (ec) {
        auto logger = client::logging::Logger::getBootLogger();
        if (logger) {
            logger->error("Failed to create config directory: {}", ec.message());
        }
        return false;
    }

    auto configPath = getConfigFilePath();
    std::ofstream file(configPath);
    if (!file.is_open()) {
        auto logger = client::logging::Logger::getBootLogger();
        if (logger) {
            logger->error("Failed to open config file for writing: {}", configPath.string());
        }
        return false;
    }

    file << toJson();
    file.close();

    auto logger = client::logging::Logger::getBootLogger();
    if (logger) {
        logger->info("Server config saved to {}", configPath.string());
    }
    return true;
}

bool ServerConfigManager::parseJson(const std::string& json)
{
    std::lock_guard<std::mutex> lock(_mutex);

    // Simple JSON parser (no external dependency)
    // Format: {"host":"...","tcpPort":...,"udpPort":...,"voicePort":...}

    auto extractString = [&json](const std::string& key) -> std::string {
        std::regex re("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
        std::smatch match;
        if (std::regex_search(json, match, re) && match.size() > 1) {
            return match[1].str();
        }
        return "";
    };

    auto extractInt = [&json](const std::string& key) -> int {
        std::regex re("\"" + key + "\"\\s*:\\s*(\\d+)");
        std::smatch match;
        if (std::regex_search(json, match, re) && match.size() > 1) {
            try {
                return std::stoi(match[1].str());
            } catch (...) {
                return 0;
            }
        }
        return 0;
    };

    std::string host = extractString("host");
    int tcpPort = extractInt("tcpPort");
    int udpPort = extractInt("udpPort");
    int voicePort = extractInt("voicePort");

    // Only update if valid values found
    if (!host.empty()) {
        _config.host = host;
    }
    if (tcpPort > 0 && tcpPort <= 65535) {
        _config.tcpPort = static_cast<uint16_t>(tcpPort);
    }
    if (udpPort > 0 && udpPort <= 65535) {
        _config.udpPort = static_cast<uint16_t>(udpPort);
    }
    if (voicePort > 0 && voicePort <= 65535) {
        _config.voicePort = static_cast<uint16_t>(voicePort);
    }

    return true;
}

std::string ServerConfigManager::toJson() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    std::ostringstream oss;
    oss << "{\n"
        << "    \"host\": \"" << _config.host << "\",\n"
        << "    \"tcpPort\": " << _config.tcpPort << ",\n"
        << "    \"udpPort\": " << _config.udpPort << ",\n"
        << "    \"voicePort\": " << _config.voicePort << "\n"
        << "}\n";

    return oss.str();
}

} // namespace config
