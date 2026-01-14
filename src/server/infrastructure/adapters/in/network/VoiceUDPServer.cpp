/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** VoiceUDPServer - Implementation
*/

#include "infrastructure/adapters/in/network/VoiceUDPServer.hpp"
#include "infrastructure/logging/Logger.hpp"
#include "Protocol.hpp"

#ifdef _WIN32
    #include <winsock2.h>
    #include <mstcpip.h>
#endif

namespace infrastructure::adapters::in::network {

    VoiceUDPServer::VoiceUDPServer(boost::asio::io_context& io_ctx,
                                   std::shared_ptr<SessionManager> sessionManager)
        : _io_ctx(io_ctx),
          _socket(io_ctx, udp::endpoint(udp::v4(), VOICE_UDP_PORT)),
          _sessionManager(sessionManager) {

        // Windows: disable ICMP Port Unreachable errors on UDP
        #ifdef _WIN32
            BOOL bNewBehavior = FALSE;
            DWORD dwBytesReturned = 0;
            WSAIoctl(
                _socket.native_handle(), SIO_UDP_CONNRESET,
                &bNewBehavior, sizeof(bNewBehavior),
                NULL, 0, &dwBytesReturned, NULL, NULL
            );
        #endif

        server::logging::Logger::getNetworkLogger()->info(
            "VoiceUDPServer initialized on port {}", VOICE_UDP_PORT);
    }

    void VoiceUDPServer::start() {
        server::logging::Logger::getNetworkLogger()->info("VoiceUDPServer started");
        do_read();
    }

    void VoiceUDPServer::stop() {
        _socket.close();
        server::logging::Logger::getNetworkLogger()->info("VoiceUDPServer stopped");
    }

    std::string VoiceUDPServer::endpointToString(const udp::endpoint& ep) const {
        return ep.address().to_string() + ":" + std::to_string(ep.port());
    }

    void VoiceUDPServer::sendTo(const udp::endpoint& endpoint, const void* data, size_t size) {
        auto buf = std::make_shared<std::vector<uint8_t>>(
            static_cast<const uint8_t*>(data),
            static_cast<const uint8_t*>(data) + size
        );

        _socket.async_send_to(
            boost::asio::buffer(buf->data(), buf->size()),
            endpoint,
            [buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    server::logging::Logger::getNetworkLogger()->error(
                        "Voice send error: {}", ec.message());
                }
            }
        );
    }

    void VoiceUDPServer::sendVoiceJoinAck(const udp::endpoint& endpoint, uint8_t playerId) {
        const size_t totalSize = UDPHeader::WIRE_SIZE + VoiceJoinAck::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::VoiceJoinAck),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        VoiceJoinAck ack{.player_id = playerId};
        ack.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        sendTo(endpoint, buf.data(), buf.size());

        server::logging::Logger::getNetworkLogger()->debug(
            "VoiceJoinAck sent to {} (playerId={})",
            endpointToString(endpoint), static_cast<int>(playerId));
    }

    void VoiceUDPServer::do_read() {
        _socket.async_receive_from(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            _remote_endpoint,
            [this](const boost::system::error_code& error, std::size_t bytes) {
                handle_receive(error, bytes);
            }
        );
    }

    void VoiceUDPServer::handle_receive(const boost::system::error_code& error, std::size_t bytes) {
        if (error) {
            if (error != boost::asio::error::operation_aborted) {
                server::logging::Logger::getNetworkLogger()->error(
                    "Voice receive error: {}", error.message());
                do_read();
            }
            return;
        }

        if (bytes < UDPHeader::WIRE_SIZE) {
            do_read();
            return;
        }

        auto headOpt = UDPHeader::from_bytes(_readBuffer, bytes);
        if (!headOpt) {
            do_read();
            return;
        }

        UDPHeader head = *headOpt;
        size_t payload_size = bytes - UDPHeader::WIRE_SIZE;
        const uint8_t* payload = reinterpret_cast<const uint8_t*>(_readBuffer) + UDPHeader::WIRE_SIZE;

        switch (static_cast<MessageType>(head.type)) {
            case MessageType::VoiceJoin:
                handleVoiceJoin(_remote_endpoint, payload, payload_size);
                break;

            case MessageType::VoiceLeave:
                handleVoiceLeave(_remote_endpoint);
                break;

            case MessageType::VoiceFrame:
                handleVoiceFrame(_remote_endpoint, payload, payload_size);
                break;

            case MessageType::VoiceMute:
                handleVoiceMute(_remote_endpoint, payload, payload_size);
                break;

            default:
                // Unknown message type for voice server
                break;
        }

        do_read();
    }

    void VoiceUDPServer::handleVoiceJoin(const udp::endpoint& endpoint,
                                         const uint8_t* payload, size_t payload_size) {
        if (payload_size < VoiceJoin::WIRE_SIZE) {
            server::logging::Logger::getNetworkLogger()->warn(
                "VoiceJoin packet too small from {}", endpointToString(endpoint));
            return;
        }

        auto joinOpt = VoiceJoin::from_bytes(payload, payload_size);
        if (!joinOpt) {
            server::logging::Logger::getNetworkLogger()->warn(
                "Invalid VoiceJoin packet from {}", endpointToString(endpoint));
            return;
        }

        // Validate token via SessionManager (reuse game auth)
        // Note: For voice, we use a different endpoint (different port),
        // so we validate the token but don't bind it again
        auto sessionOpt = _sessionManager->validateToken(joinOpt->token);
        if (!sessionOpt) {
            server::logging::Logger::getNetworkLogger()->warn(
                "VoiceJoin with invalid token from {}", endpointToString(endpoint));
            return;
        }

        std::string roomCode(joinOpt->roomCode, ROOM_CODE_LEN);
        std::string endpointStr = endpointToString(endpoint);
        uint8_t playerId = sessionOpt->playerId;

        // Add to voice channel
        {
            std::lock_guard<std::mutex> lock(_voiceMutex);

            // Remove from previous room if any
            auto it = _endpointToRoom.find(endpointStr);
            if (it != _endpointToRoom.end()) {
                _voiceChannels[it->second].erase(endpointStr);
            }

            // Add to new room
            _voiceChannels[roomCode].insert(endpointStr);
            _endpointToRoom[endpointStr] = roomCode;
            _endpointCache[endpointStr] = endpoint;
            _endpointToPlayerId[endpointStr] = playerId;
        }

        sendVoiceJoinAck(endpoint, playerId);

        server::logging::Logger::getNetworkLogger()->info(
            "Player {} joined voice channel for room '{}'",
            static_cast<int>(playerId), roomCode);
    }

    void VoiceUDPServer::handleVoiceLeave(const udp::endpoint& endpoint) {
        std::string endpointStr = endpointToString(endpoint);

        std::lock_guard<std::mutex> lock(_voiceMutex);

        auto it = _endpointToRoom.find(endpointStr);
        if (it != _endpointToRoom.end()) {
            std::string roomCode = it->second;

            // Get player ID before removing
            uint8_t playerId = 0;
            auto pidIt = _endpointToPlayerId.find(endpointStr);
            if (pidIt != _endpointToPlayerId.end()) {
                playerId = pidIt->second;
            }

            _voiceChannels[roomCode].erase(endpointStr);
            _endpointToRoom.erase(endpointStr);
            _endpointCache.erase(endpointStr);
            _endpointToPlayerId.erase(endpointStr);

            // Clean up empty rooms
            if (_voiceChannels[roomCode].empty()) {
                _voiceChannels.erase(roomCode);
            }

            server::logging::Logger::getNetworkLogger()->info(
                "Player {} left voice channel for room '{}'",
                static_cast<int>(playerId), roomCode);
        }
    }

    void VoiceUDPServer::handleVoiceFrame(const udp::endpoint& endpoint,
                                          const uint8_t* payload, size_t payload_size) {
        if (payload_size < VoiceFrame::HEADER_SIZE) {
            return;  // Invalid frame, silently ignore
        }

        std::string endpointStr = endpointToString(endpoint);
        std::string roomCode;

        {
            std::lock_guard<std::mutex> lock(_voiceMutex);
            auto it = _endpointToRoom.find(endpointStr);
            if (it == _endpointToRoom.end()) {
                // Not in any voice channel, ignore
                return;
            }
            roomCode = it->second;
        }

        // Relay the entire message (header + payload) to other room members
        // We need to reconstruct the full message with UDPHeader
        const size_t totalSize = UDPHeader::WIRE_SIZE + payload_size;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::VoiceFrame),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());
        std::memcpy(buf.data() + UDPHeader::WIRE_SIZE, payload, payload_size);

        relayVoiceFrame(roomCode, endpoint, buf.data(), buf.size());
    }

    void VoiceUDPServer::handleVoiceMute(const udp::endpoint& endpoint,
                                         const uint8_t* payload, size_t payload_size) {
        if (payload_size < VoiceMute::WIRE_SIZE) {
            return;
        }

        auto muteOpt = VoiceMute::from_bytes(payload, payload_size);
        if (!muteOpt) {
            return;
        }

        std::string endpointStr = endpointToString(endpoint);
        std::string roomCode;
        uint8_t playerId = muteOpt->player_id;

        {
            std::lock_guard<std::mutex> lock(_voiceMutex);
            auto it = _endpointToRoom.find(endpointStr);
            if (it == _endpointToRoom.end()) {
                return;
            }
            roomCode = it->second;
        }

        broadcastMuteStatus(roomCode, playerId, muteOpt->muted != 0);

        server::logging::Logger::getNetworkLogger()->debug(
            "Player {} {} in room '{}'",
            static_cast<int>(playerId),
            muteOpt->muted ? "muted" : "unmuted",
            roomCode);
    }

    void VoiceUDPServer::relayVoiceFrame(const std::string& roomCode,
                                         const udp::endpoint& senderEndpoint,
                                         const void* data, size_t size) {
        std::string senderStr = endpointToString(senderEndpoint);
        std::vector<udp::endpoint> recipients;
        uint8_t senderId = 0;

        {
            std::lock_guard<std::mutex> lock(_voiceMutex);
            auto channelIt = _voiceChannels.find(roomCode);
            if (channelIt == _voiceChannels.end()) {
                server::logging::Logger::getNetworkLogger()->debug(
                    "VoiceFrame relay: room '{}' not found", roomCode);
                return;
            }

            // Get sender's player ID
            auto pidIt = _endpointToPlayerId.find(senderStr);
            if (pidIt != _endpointToPlayerId.end()) {
                senderId = pidIt->second;
            }

            // Get all endpoints except sender
            for (const auto& epStr : channelIt->second) {
                if (epStr != senderStr) {
                    auto cacheIt = _endpointCache.find(epStr);
                    if (cacheIt != _endpointCache.end()) {
                        recipients.push_back(cacheIt->second);
                    }
                }
            }
        }

        if (recipients.empty()) {
            server::logging::Logger::getNetworkLogger()->debug(
                "VoiceFrame from P{} in room '{}': no other recipients",
                static_cast<int>(senderId), roomCode);
            return;
        }

        server::logging::Logger::getNetworkLogger()->debug(
            "VoiceFrame relay: P{} -> {} recipients in room '{}' ({} bytes)",
            static_cast<int>(senderId), recipients.size(), roomCode, size);

        // Send to all recipients (outside lock)
        for (const auto& ep : recipients) {
            sendTo(ep, data, size);
        }
    }

    void VoiceUDPServer::broadcastMuteStatus(const std::string& roomCode,
                                             uint8_t playerId, bool muted) {
        const size_t totalSize = UDPHeader::WIRE_SIZE + VoiceMute::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::VoiceMute),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        VoiceMute vm{.player_id = playerId, .muted = static_cast<uint8_t>(muted ? 1 : 0)};
        vm.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        std::vector<udp::endpoint> recipients;

        {
            std::lock_guard<std::mutex> lock(_voiceMutex);
            auto channelIt = _voiceChannels.find(roomCode);
            if (channelIt == _voiceChannels.end()) {
                return;
            }

            for (const auto& epStr : channelIt->second) {
                auto cacheIt = _endpointCache.find(epStr);
                if (cacheIt != _endpointCache.end()) {
                    recipients.push_back(cacheIt->second);
                }
            }
        }

        for (const auto& ep : recipients) {
            sendTo(ep, buf.data(), buf.size());
        }
    }

}
