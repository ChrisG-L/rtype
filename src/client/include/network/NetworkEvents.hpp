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

namespace client::network
{
    // TCP-specific events
    struct TCPConnectedEvent {};
    struct TCPDisconnectedEvent {};
    struct TCPAuthSuccessEvent {};
    struct TCPAuthFailedEvent { std::string message; };
    struct TCPErrorEvent { std::string message; };

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
        TCPErrorEvent
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
