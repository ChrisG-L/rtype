/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** NetworkStats implementation
*/

#include "infrastructure/network/NetworkStats.hpp"
#include <algorithm>
#include <numeric>

namespace infrastructure::network {

// ═══════════════════════════════════════════════════════════════════════════
// Global Stats
// ═══════════════════════════════════════════════════════════════════════════

void NetworkStats::addBytesSent(size_t bytes) {
    _totalBytesSent.fetch_add(bytes, std::memory_order_relaxed);
}

void NetworkStats::addBytesReceived(size_t bytes) {
    _totalBytesReceived.fetch_add(bytes, std::memory_order_relaxed);
}

// ═══════════════════════════════════════════════════════════════════════════
// Per-Player Stats
// ═══════════════════════════════════════════════════════════════════════════

void NetworkStats::registerPlayer(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(_playerStatsMutex);

    if (_playerStats.find(endpoint) == _playerStats.end()) {
        PlayerNetworkStats stats;
        stats.endpoint = endpoint;
        stats.connectedAt = std::chrono::steady_clock::now();
        stats.lastUpdate = stats.connectedAt;
        _playerStats[endpoint] = stats;
    }
}

void NetworkStats::unregisterPlayer(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(_playerStatsMutex);
    _playerStats.erase(endpoint);
}

void NetworkStats::addBytesSentTo(const std::string& endpoint, size_t bytes) {
    std::lock_guard<std::mutex> lock(_playerStatsMutex);

    auto it = _playerStats.find(endpoint);
    if (it != _playerStats.end()) {
        it->second.bytesSent += bytes;
        it->second.lastUpdate = std::chrono::steady_clock::now();
    }
}

void NetworkStats::addBytesReceivedFrom(const std::string& endpoint, size_t bytes) {
    std::lock_guard<std::mutex> lock(_playerStatsMutex);

    auto it = _playerStats.find(endpoint);
    if (it != _playerStats.end()) {
        it->second.bytesReceived += bytes;
        it->second.lastUpdate = std::chrono::steady_clock::now();
    }
}

void NetworkStats::updatePlayerRTT(const std::string& endpoint, uint32_t rttMs) {
    std::lock_guard<std::mutex> lock(_playerStatsMutex);

    auto it = _playerStats.find(endpoint);
    if (it != _playerStats.end()) {
        auto& stats = it->second;

        // Update current RTT
        stats.rttCurrent = rttMs;

        // Update max RTT
        if (rttMs > stats.rttMax) {
            stats.rttMax = rttMs;
        }

        // Update average RTT (cumulative moving average)
        stats.rttSampleCount++;
        stats.rttSum += rttMs;
        stats.rttAverage = static_cast<uint32_t>(stats.rttSum / stats.rttSampleCount);

        stats.lastUpdate = std::chrono::steady_clock::now();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Rate Calculation
// ═══════════════════════════════════════════════════════════════════════════

void NetworkStats::calculateRates() {
    // Calculate global rates
    uint64_t currentSent = _totalBytesSent.load();
    uint64_t currentReceived = _totalBytesReceived.load();
    uint64_t lastSent = _lastBytesSent.exchange(currentSent);
    uint64_t lastReceived = _lastBytesReceived.exchange(currentReceived);

    double sendRate = static_cast<double>(currentSent - lastSent);
    double receiveRate = static_cast<double>(currentReceived - lastReceived);

    _currentSendRate.store(sendRate);
    _currentReceiveRate.store(receiveRate);

    // Update rate history for moving average
    _sendRateHistory[_rateHistoryIndex] = sendRate;
    _receiveRateHistory[_rateHistoryIndex] = receiveRate;
    _rateHistoryIndex = (_rateHistoryIndex + 1) % RATE_HISTORY_SIZE;
    if (_rateHistoryCount < RATE_HISTORY_SIZE) {
        _rateHistoryCount++;
    }

    // Calculate moving average
    if (_rateHistoryCount > 0) {
        double sendSum = 0.0;
        double receiveSum = 0.0;
        for (size_t i = 0; i < _rateHistoryCount; ++i) {
            sendSum += _sendRateHistory[i];
            receiveSum += _receiveRateHistory[i];
        }
        _averageSendRate.store(sendSum / static_cast<double>(_rateHistoryCount));
        _averageReceiveRate.store(receiveSum / static_cast<double>(_rateHistoryCount));
    }

    // Calculate per-player rates
    std::lock_guard<std::mutex> lock(_playerStatsMutex);

    for (auto& [endpoint, stats] : _playerStats) {
        // OUT rate (server → player)
        uint64_t outDelta = stats.bytesSent - stats.lastBytesSent;
        stats.lastBytesSent = stats.bytesSent;
        stats.outCurrent = static_cast<double>(outDelta);

        // Update peak
        if (stats.outCurrent > stats.outPeak) {
            stats.outPeak = stats.outCurrent;
        }

        // Simple exponential moving average for outAverage
        // α = 0.2 means ~5 samples to reach 63% of a step change
        constexpr double alpha = 0.2;
        stats.outAverage = alpha * stats.outCurrent + (1.0 - alpha) * stats.outAverage;

        // IN rate (player → server)
        uint64_t inDelta = stats.bytesReceived - stats.lastBytesReceived;
        stats.lastBytesReceived = stats.bytesReceived;
        stats.inCurrent = static_cast<double>(inDelta);

        // Simple exponential moving average for inAverage
        stats.inAverage = alpha * stats.inCurrent + (1.0 - alpha) * stats.inAverage;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Getters
// ═══════════════════════════════════════════════════════════════════════════

uint32_t NetworkStats::getGlobalAverageRTT() const {
    std::lock_guard<std::mutex> lock(_playerStatsMutex);

    if (_playerStats.empty()) {
        return 0;
    }

    uint64_t sum = 0;
    size_t count = 0;
    for (const auto& [endpoint, stats] : _playerStats) {
        if (stats.rttSampleCount > 0) {
            sum += stats.rttAverage;
            count++;
        }
    }

    return count > 0 ? static_cast<uint32_t>(sum / count) : 0;
}

std::optional<PlayerNetworkStats> NetworkStats::getPlayerStats(const std::string& endpoint) const {
    std::lock_guard<std::mutex> lock(_playerStatsMutex);

    auto it = _playerStats.find(endpoint);
    if (it != _playerStats.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::pair<std::string, PlayerNetworkStats>> NetworkStats::getAllPlayerStats() const {
    std::lock_guard<std::mutex> lock(_playerStatsMutex);

    std::vector<std::pair<std::string, PlayerNetworkStats>> result;
    result.reserve(_playerStats.size());
    for (const auto& [endpoint, stats] : _playerStats) {
        result.emplace_back(endpoint, stats);
    }
    return result;
}

RoomNetworkStats NetworkStats::getRoomStats(const std::vector<std::string>& endpoints) const {
    std::lock_guard<std::mutex> lock(_playerStatsMutex);

    RoomNetworkStats roomStats;

    if (endpoints.empty()) {
        return roomStats;
    }

    uint64_t rttSum = 0;
    size_t rttCount = 0;

    for (const auto& endpoint : endpoints) {
        auto it = _playerStats.find(endpoint);
        if (it != _playerStats.end()) {
            const auto& stats = it->second;

            roomStats.outCurrent += stats.outCurrent;
            roomStats.outAverage += stats.outAverage;
            roomStats.inCurrent += stats.inCurrent;
            roomStats.inAverage += stats.inAverage;
            roomStats.playerCount++;

            if (stats.rttSampleCount > 0) {
                rttSum += stats.rttAverage;
                rttCount++;
            }
        }
    }

    if (rttCount > 0) {
        roomStats.rttAverage = static_cast<uint32_t>(rttSum / rttCount);
    }

    return roomStats;
}

} // namespace infrastructure::network
