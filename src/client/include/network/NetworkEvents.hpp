/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** NetworkEvents - Thread-safe event queue for network callbacks
*/

#ifndef NETWORKEVENTS_HPP_
#define NETWORKEVENTS_HPP_

#include <queue>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <array>
#include <cstdint>
#include "Protocol.hpp"

namespace client::network
{
    // TCP-specific events
    struct TCPConnectedEvent {};
    struct TCPDisconnectedEvent {};
    struct TCPAuthSuccessEvent {
        VersionInfo serverVersion;  // Server version for compatibility check
    };
    struct TCPAuthFailedEvent { std::string message; };
    struct TCPErrorEvent { std::string message; };

    // Version mismatch event (client needs update)
    struct TCPVersionMismatchEvent {
        VersionInfo clientVersion;
        VersionInfo serverVersion;
        int commitsBehind;  // Number of commits behind (-1 = unknown/too old, 0+ = exact count)
    };

    // Room-specific events (TCP)
    struct RoomPlayerInfo {
        uint8_t slotId;
        std::string displayName;
        std::string email;  // Added for kick functionality (Phase 2)
        bool isReady;
        bool isHost;
        uint8_t shipSkin = 1;  // Ship skin variant (1-6)
    };

    struct TCPRoomCreatedEvent {
        std::string roomCode;
    };
    struct TCPRoomCreateFailedEvent {
        std::string errorCode;
        std::string message;
    };
    struct TCPRoomJoinedEvent {
        uint8_t slotId;
        std::string roomName;
        std::string roomCode;
        uint8_t maxPlayers;
        bool isHost;
        std::vector<RoomPlayerInfo> players;  // Initial player list (fixes race condition)
    };
    struct TCPRoomJoinFailedEvent {
        std::string errorCode;
        std::string message;
    };
    struct TCPRoomLeftEvent {};
    struct TCPRoomUpdateEvent {
        std::string roomName;
        std::string roomCode;
        uint8_t maxPlayers;
        uint16_t gameSpeedPercent = 100;  // 50-200, default 100
        std::vector<RoomPlayerInfo> players;
    };
    struct TCPGameStartingEvent {
        uint8_t countdown;
    };
    struct TCPReadyChangedEvent {
        bool isReady;
    };

    // Kick events (Phase 2)
    struct TCPPlayerKickedEvent {
        std::string reason;
    };
    struct TCPKickSuccessEvent {};

    // Room Browser events (Phase 2)
    struct RoomBrowserInfo {
        std::string code;
        std::string name;
        uint8_t currentPlayers;
        uint8_t maxPlayers;
    };

    struct TCPRoomListEvent {
        std::vector<RoomBrowserInfo> rooms;
    };

    struct TCPQuickJoinFailedEvent {
        std::string errorCode;
        std::string message;
    };

    // User Settings events (Phase 2)
    struct TCPUserSettingsEvent {
        bool found;                         // true if settings found in DB
        std::string colorBlindMode;         // "none", "protanopia", etc.
        float gameSpeed;                    // 0.5 to 2.0
        std::array<uint8_t, 26> keyBindings; // 13 actions × 2 keys (matches KEY_BINDINGS_COUNT)
        uint8_t shipSkin;                   // Ship skin variant (1-6)
        // Voice settings
        uint8_t voiceMode;                  // 0 = PTT, 1 = VAD
        uint8_t vadThreshold;               // 0-100
        uint8_t micGain;                    // 0-200
        uint8_t voiceVolume;                // 0-100
        std::string audioInputDevice;       // Input device name (empty = auto)
        std::string audioOutputDevice;      // Output device name (empty = auto)
        // Chat settings
        bool keepChatOpenAfterSend;         // If true, chat stays open after sending message
    };

    struct TCPSaveSettingsResultEvent {
        bool success;
        std::string message;
    };

    // Chat events (Phase 2)
    struct ChatMessageInfo {
        std::string displayName;
        std::string message;
        uint32_t timestamp;
    };

    struct TCPChatMessageEvent {
        std::string displayName;
        std::string message;
        uint32_t timestamp;
    };

    struct TCPChatHistoryEvent {
        std::vector<ChatMessageInfo> messages;
    };

    // Leaderboard events (Phase 3)
    struct LeaderboardDataEvent {
        LeaderboardDataResponse response;
        std::vector<LeaderboardEntryWire> entries;
    };

    struct PlayerStatsDataEvent {
        PlayerStatsWire stats;
    };

    struct AchievementsDataEvent {
        uint32_t bitfield;
    };

    struct GameHistoryDataEvent {
        std::vector<GameHistoryEntryWire> entries;
    };

    // UDP-specific events
    struct UDPConnectedEvent { uint8_t playerId; };
    struct UDPDisconnectedEvent {};
    struct UDPErrorEvent { std::string message; };
    struct UDPPlayerJoinedEvent { uint8_t playerId; };
    struct UDPPlayerLeftEvent { uint8_t playerId; };
    struct UDPJoinGameAckEvent { uint8_t playerId; };
    struct UDPJoinGameNackEvent { std::string reason; };
    struct UDPPlayerDamagedEvent {
        uint8_t playerId;
        uint8_t damage;
        uint8_t newHealth;
    };

    // Type aliases for event variants
    using TCPEvent = std::variant<
        TCPConnectedEvent,
        TCPDisconnectedEvent,
        TCPAuthSuccessEvent,
        TCPAuthFailedEvent,
        TCPErrorEvent,
        TCPVersionMismatchEvent,
        // Room events
        TCPRoomCreatedEvent,
        TCPRoomCreateFailedEvent,
        TCPRoomJoinedEvent,
        TCPRoomJoinFailedEvent,
        TCPRoomLeftEvent,
        TCPRoomUpdateEvent,
        TCPGameStartingEvent,
        TCPReadyChangedEvent,
        // Kick events (Phase 2)
        TCPPlayerKickedEvent,
        TCPKickSuccessEvent,
        // Room Browser events (Phase 2)
        TCPRoomListEvent,
        TCPQuickJoinFailedEvent,
        // User Settings events (Phase 2)
        TCPUserSettingsEvent,
        TCPSaveSettingsResultEvent,
        // Chat events (Phase 2)
        TCPChatMessageEvent,
        TCPChatHistoryEvent,
        // Leaderboard events (Phase 3)
        LeaderboardDataEvent,
        PlayerStatsDataEvent,
        AchievementsDataEvent,
        GameHistoryDataEvent
    >;

    using UDPEvent = std::variant<
        UDPConnectedEvent,
        UDPDisconnectedEvent,
        UDPErrorEvent,
        UDPPlayerJoinedEvent,
        UDPPlayerLeftEvent,
        UDPJoinGameAckEvent,
        UDPJoinGameNackEvent,
        UDPPlayerDamagedEvent
    >;

    // Thread-safe event queue
    template<typename EventType>
    class EventQueue
    {
    public:
        void push(EventType event)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _events.push(std::move(event));
        }

        std::optional<EventType> poll()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_events.empty()) {
                return std::nullopt;
            }
            EventType event = std::move(_events.front());
            _events.pop();
            return event;
        }

        void clear()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            while (!_events.empty()) {
                _events.pop();
            }
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lock(_mutex);
            return _events.empty();
        }

    private:
        std::queue<EventType> _events;
        mutable std::mutex _mutex;
    };

}

#endif /* !NETWORKEVENTS_HPP_ */
