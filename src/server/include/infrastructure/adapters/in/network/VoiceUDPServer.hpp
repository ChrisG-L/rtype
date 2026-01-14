/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** VoiceUDPServer - UDP server for voice chat relay (port 4126)
*/

#ifndef VOICEUDPSERVER_HPP_
#define VOICEUDPSERVER_HPP_

#include <array>
#include <boost/asio.hpp>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include "Protocol.hpp"
#include "infrastructure/session/SessionManager.hpp"

namespace infrastructure::adapters::in::network {
    using boost::asio::ip::udp;
    using infrastructure::session::SessionManager;

    /**
     * VoiceUDPServer handles voice chat relay between players in the same room.
     *
     * Architecture:
     * - Listens on port 4126 (separate from game UDP on 4124)
     * - Authenticates clients via SessionToken (same as game)
     * - Relays VoiceFrame packets to all other players in the same room
     * - No audio processing - pure relay (Opus encoding/decoding is client-side)
     *
     * Protocol:
     * - VoiceJoin: Client joins voice channel for a room
     * - VoiceJoinAck: Server confirms with player_id
     * - VoiceLeave: Client leaves voice channel
     * - VoiceFrame: Audio data relayed to room members
     * - VoiceMute: Mute/unmute notification
     */
    class VoiceUDPServer {
    public:
        VoiceUDPServer(boost::asio::io_context& io_ctx,
                       std::shared_ptr<SessionManager> sessionManager);

        void start();
        void stop();

    private:
        void do_read();
        void handle_receive(const boost::system::error_code& error, std::size_t bytes);

        // Send helpers
        void sendTo(const udp::endpoint& endpoint, const void* data, size_t size);
        void sendVoiceJoinAck(const udp::endpoint& endpoint, uint8_t playerId);

        // Message handlers
        void handleVoiceJoin(const udp::endpoint& endpoint,
                            const uint8_t* payload, size_t payload_size);
        void handleVoiceLeave(const udp::endpoint& endpoint);
        void handleVoiceFrame(const udp::endpoint& endpoint,
                             const uint8_t* payload, size_t payload_size);
        void handleVoiceMute(const udp::endpoint& endpoint,
                            const uint8_t* payload, size_t payload_size);

        // Relay voice frame to all other players in the room
        void relayVoiceFrame(const std::string& roomCode,
                            const udp::endpoint& senderEndpoint,
                            const void* data, size_t size);

        // Broadcast mute status to room
        void broadcastMuteStatus(const std::string& roomCode,
                                uint8_t playerId, bool muted);

        // Helper to convert endpoint to string key
        std::string endpointToString(const udp::endpoint& ep) const;

        boost::asio::io_context& _io_ctx;
        udp::socket _socket;  // Port 4126
        udp::endpoint _remote_endpoint;
        char _readBuffer[BUFFER_SIZE];

        std::shared_ptr<SessionManager> _sessionManager;

        // Voice channel membership: roomCode -> set of endpoints (as strings)
        std::unordered_map<std::string, std::unordered_set<std::string>> _voiceChannels;

        // Reverse lookup: endpoint string -> roomCode
        std::unordered_map<std::string, std::string> _endpointToRoom;

        // Endpoint string -> actual endpoint (for sending)
        std::unordered_map<std::string, udp::endpoint> _endpointCache;

        // Endpoint string -> player_id
        std::unordered_map<std::string, uint8_t> _endpointToPlayerId;

        // Mutex for thread-safe access to voice channel data
        mutable std::mutex _voiceMutex;
    };
}

#endif /* !VOICEUDPSERVER_HPP_ */
