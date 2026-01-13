/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** NetworkStats - Network metrics tracking for monitoring
*/

#ifndef NETWORKSTATS_HPP_
#define NETWORKSTATS_HPP_

#include <atomic>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <string>
#include <array>
#include <vector>
#include <optional>
#include <cstdint>

namespace infrastructure::network {

/**
 * @brief Per-player network statistics
 */
struct PlayerNetworkStats {
    std::string endpoint;

    // RTT (milliseconds)
    uint32_t rttCurrent{0};
    uint32_t rttAverage{0};
    uint32_t rttMax{0};
    uint32_t rttSampleCount{0};
    uint64_t rttSum{0};  // For calculating average

    // Bandwidth OUT (server → player)
    uint64_t bytesSent{0};
    uint64_t lastBytesSent{0};
    double outCurrent{0.0};   // bytes/s
    double outAverage{0.0};   // bytes/s
    double outPeak{0.0};      // bytes/s max observed

    // Bandwidth IN (player → server)
    uint64_t bytesReceived{0};
    uint64_t lastBytesReceived{0};
    double inCurrent{0.0};    // bytes/s
    double inAverage{0.0};    // bytes/s

    std::chrono::steady_clock::time_point lastUpdate;
    std::chrono::steady_clock::time_point connectedAt;
};

/**
 * @brief Aggregated network stats for a room
 */
struct RoomNetworkStats {
    double outCurrent{0.0};
    double outAverage{0.0};
    double inCurrent{0.0};
    double inAverage{0.0};
    uint32_t rttAverage{0};
    size_t playerCount{0};
};

/**
 * @brief Thread-safe network statistics collector
 *
 * Tracks bandwidth and RTT metrics both globally and per-player.
 * Uses atomic counters for global stats (lock-free) and mutex for per-player stats.
 */
class NetworkStats {
public:
    NetworkStats() = default;
    ~NetworkStats() = default;

    // Disable copy
    NetworkStats(const NetworkStats&) = delete;
    NetworkStats& operator=(const NetworkStats&) = delete;

    // ═══════════════════════════════════════════════════════════════════
    // Global Stats (thread-safe atomics)
    // ═══════════════════════════════════════════════════════════════════

    void addBytesSent(size_t bytes);
    void addBytesReceived(size_t bytes);

    // ═══════════════════════════════════════════════════════════════════
    // Per-Player Stats (mutex-protected)
    // ═══════════════════════════════════════════════════════════════════

    void addBytesSentTo(const std::string& endpoint, size_t bytes);
    void addBytesReceivedFrom(const std::string& endpoint, size_t bytes);
    void updatePlayerRTT(const std::string& endpoint, uint32_t rttMs);
    void registerPlayer(const std::string& endpoint);
    void unregisterPlayer(const std::string& endpoint);

    // ═══════════════════════════════════════════════════════════════════
    // Rate Calculation (called every second by timer)
    // ═══════════════════════════════════════════════════════════════════

    void calculateRates();

    // ═══════════════════════════════════════════════════════════════════
    // Global Getters
    // ═══════════════════════════════════════════════════════════════════

    double getCurrentSendRate() const { return _currentSendRate.load(); }
    double getAverageSendRate() const { return _averageSendRate.load(); }
    double getCurrentReceiveRate() const { return _currentReceiveRate.load(); }
    double getAverageReceiveRate() const { return _averageReceiveRate.load(); }
    uint32_t getGlobalAverageRTT() const;

    uint64_t getTotalBytesSent() const { return _totalBytesSent.load(); }
    uint64_t getTotalBytesReceived() const { return _totalBytesReceived.load(); }

    // ═══════════════════════════════════════════════════════════════════
    // Per-Player Getters
    // ═══════════════════════════════════════════════════════════════════

    std::optional<PlayerNetworkStats> getPlayerStats(const std::string& endpoint) const;
    std::vector<std::pair<std::string, PlayerNetworkStats>> getAllPlayerStats() const;

    // ═══════════════════════════════════════════════════════════════════
    // Room Aggregation
    // ═══════════════════════════════════════════════════════════════════

    RoomNetworkStats getRoomStats(const std::vector<std::string>& endpoints) const;

private:
    // Global counters (atomics for lock-free access)
    std::atomic<uint64_t> _totalBytesSent{0};
    std::atomic<uint64_t> _totalBytesReceived{0};
    std::atomic<uint64_t> _lastBytesSent{0};
    std::atomic<uint64_t> _lastBytesReceived{0};

    // Global rates
    std::atomic<double> _currentSendRate{0.0};
    std::atomic<double> _currentReceiveRate{0.0};
    std::atomic<double> _averageSendRate{0.0};
    std::atomic<double> _averageReceiveRate{0.0};

    // Rate history for moving average (10 seconds)
    static constexpr size_t RATE_HISTORY_SIZE = 10;
    std::array<double, RATE_HISTORY_SIZE> _sendRateHistory{};
    std::array<double, RATE_HISTORY_SIZE> _receiveRateHistory{};
    size_t _rateHistoryIndex{0};
    size_t _rateHistoryCount{0};  // How many samples we have (up to RATE_HISTORY_SIZE)

    // Per-player stats (protected by mutex)
    mutable std::mutex _playerStatsMutex;
    std::unordered_map<std::string, PlayerNetworkStats> _playerStats;

    std::chrono::steady_clock::time_point _startTime{std::chrono::steady_clock::now()};
};

} // namespace infrastructure::network

#endif /* !NETWORKSTATS_HPP_ */
