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

namespace client::network
{
    // TCP-specific events
    struct TCPConnectedEvent {};
    struct TCPDisconnectedEvent {};
    struct TCPAuthSuccessEvent {};
    struct TCPAuthFailedEvent { std::string message; };
    struct TCPErrorEvent { std::string message; };

    // Room-specific events (TCP)
    struct RoomPlayerInfo {
        uint8_t slotId;
        std::string displayName;
        std::string email;  // Added for kick functionality (Phase 2)
        bool isReady;
        bool isHost;
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
        std::array<uint8_t, 12> keyBindings; // 6 actions × 2 keys
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

    // UDP-specific events
    struct UDPConnectedEvent { uint8_t playerId; };
    struct UDPDisconnectedEvent {};
    struct UDPErrorEvent { std::string message; };
    struct UDPPlayerJoinedEvent { uint8_t playerId; };
    struct UDPPlayerLeftEvent { uint8_t playerId; };
    struct UDPJoinGameAckEvent { uint8_t playerId; };
    struct UDPJoinGameNackEvent { std::string reason; };
    struct UDPKickedEvent {};  // We received PlayerLeave with our own ID

    // Type aliases for event variants
    using TCPEvent = std::variant<
        TCPConnectedEvent,
        TCPDisconnectedEvent,
        TCPAuthSuccessEvent,
        TCPAuthFailedEvent,
        TCPErrorEvent,
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
        TCPChatHistoryEvent
    >;

    using UDPEvent = std::variant<
        UDPConnectedEvent,
        UDPDisconnectedEvent,
        UDPErrorEvent,
        UDPPlayerJoinedEvent,
        UDPPlayerLeftEvent,
        UDPJoinGameAckEvent,
        UDPJoinGameNackEvent,
        UDPKickedEvent
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
