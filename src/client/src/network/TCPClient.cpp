/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** TCPClient
*/

#include "network/TCPClient.hpp"
#include "Protocol.hpp"
#include "core/Logger.hpp"
#include "core/Version.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <openssl/ssl.h>

namespace client::network
{
    static constexpr int HEARTBEAT_INTERVAL_MS = 1000;
    static constexpr int HEARTBEAT_TIMEOUT_MS = 5000;  // Increased from 2000ms for stability

    TCPClient::TCPClient()
        : _ioContext()
        , _sslContext(ssl::context::tlsv12_client)
        , _socket(_ioContext, _sslContext)
        , _heartbeatTimer(_ioContext)
        , _isAuthenticated(false)
        , _isWriting(false)
    {
        initSSLContext();
        client::logging::Logger::getNetworkLogger()->debug("TCPClient created with TLS support");
    }

    void TCPClient::initSSLContext() {
        auto logger = client::logging::Logger::getNetworkLogger();

        // For development with self-signed certificates, use verify_none
        // In production, change to verify_peer and load CA certificates
        _sslContext.set_verify_mode(ssl::verify_none);

        // Load system CA certificates for future production use
        _sslContext.set_default_verify_paths();

        // Force TLS 1.2 minimum
        SSL_CTX_set_min_proto_version(_sslContext.native_handle(), TLS1_2_VERSION);

        logger->debug("SSL context initialized (TLS 1.2+)");
    }

    TCPClient::~TCPClient()
    {
        disconnect();
        if (_ioThread.joinable()) {
            _ioThread.join();
        }
    }

    void TCPClient::setOnConnected(const OnConnectedCallback& callback)
    {
        _onConnected = callback;
    }

    void TCPClient::setOnDisconnected(const OnDisconnectedCallback& callback)
    {
        _onDisconnected = callback;
    }

    void TCPClient::setOnReceive(const OnReceiveCallback& callback)
    {
        _onReceive = callback;
    }

    void TCPClient::setOnError(const OnErrorCallback& callback)
    {
        _onError = callback;
    }

    void TCPClient::connect(const std::string &host, std::uint16_t port)
    {
        auto logger = client::logging::Logger::getNetworkLogger();

        // Close any existing connection (connected or connecting)
        if (_connected.load() || _connecting.load()) {
            logger->warn("Already connected or connecting, disconnecting...");
            disconnect();
        }

        // Wait for previous IO thread to finish (if any)
        if (_ioThread.joinable()) {
            _ioThread.join();
        }

        logger->info("Connecting to {}:{} (TLS)...", host, port);

        // Store connection info for potential reconnection
        _lastHost = host;
        _lastPort = port;

        _disconnecting.store(false);
        _connecting.store(true);  // Set BEFORE asyncConnect to avoid race condition

        // Reset io_context for reuse
        _ioContext.restart();

        // Close existing SSL socket's lowest layer and recreate
        boost::system::error_code ec;
        _socket.lowest_layer().close(ec);

        // Recreate ssl::stream (cannot be reused after close)
        _socket = ssl::stream<tcp::socket>(_ioContext, _sslContext);

        try {
            tcp::resolver resolver(_ioContext);
            auto endpoints = resolver.resolve(host, std::to_string(port));

            asyncConnect(endpoints);

            _ioThread = std::jthread([this, logger]() {
                logger->debug("IO thread started");
                _ioContext.run();
                logger->debug("IO thread terminated");
            });

            logger->info("Connection initiated TCP");
        } catch (const std::exception &e) {
            logger->error("Resolution error: {}", e.what());
            _connecting.store(false);  // Reset connecting state on failure
            _eventQueue.push(TCPErrorEvent{std::string("Connexion echouee: ") + e.what()});
            if (_onError) {
                _onError(std::string("Connexion échouée: ") + e.what());
            }
        }
    }

    void TCPClient::disconnect()
    {
        // Prevent multiple disconnect calls
        if (_disconnecting.exchange(true)) {
            return;
        }

        // Check if we were connected or connecting
        bool wasConnected = _connected.load();
        bool wasConnecting = _connecting.load();

        if (!wasConnected && !wasConnecting) {
            _disconnecting.store(false);
            return;
        }

        auto logger = client::logging::Logger::getNetworkLogger();
        logger->info("Disconnecting TCP (TLS)...");

        _connecting.store(false);
        _connected.store(false);

        _heartbeatTimer.cancel();

        _ioContext.stop();

        // SSL shutdown (graceful close)
        boost::system::error_code ec;
        _socket.shutdown(ec);
        if (ec && ec != boost::asio::error::eof && ec != boost::asio::ssl::error::stream_truncated) {
            logger->debug("SSL shutdown notice: {}", ec.message());
        }
        _socket.lowest_layer().close(ec);

        _accumulator.clear();

        // Only push disconnected event if we were actually connected
        // (not just "connecting" waiting for HeartBeatAck)
        if (wasConnected) {
            _eventQueue.push(TCPDisconnectedEvent{});
            if (_onDisconnected) {
                _onDisconnected();
            }
        }

        _ioContext.restart();
        _socket = ssl::stream<tcp::socket>(_ioContext, _sslContext);

        while (!_sendQueue.empty()) {
            _sendQueue.pop();
        }
        _isWriting = false;

        logger->info("TCP disconnected successfully");
    }

    bool TCPClient::isConnected() const
    {
        return _connected.load() && _socket.lowest_layer().is_open();
    }

    bool TCPClient::isConnecting() const
    {
        return _connecting.load() && _socket.lowest_layer().is_open();
    }

    void TCPClient::asyncConnect(tcp::resolver::results_type endpoints)
    {
        // First, establish TCP connection to the lowest layer
        boost::asio::async_connect(
            _socket.lowest_layer(),  // Connect on underlying TCP socket
            endpoints,
            [this](const boost::system::error_code &error, const tcp::endpoint &) {
                handleConnect(error);
            }
        );
    }

    void TCPClient::asyncRead()
    {
        _socket.async_read_some(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            [this](const boost::system::error_code &error, std::size_t bytes) {
                handleRead(error, bytes);
            }
        );
    }

    void TCPClient::handleConnect(const boost::system::error_code &error)
    {
        // Ignore operation_aborted - expected when intentionally disconnecting
        if (error == boost::asio::error::operation_aborted) {
            return;
        }

        auto logger = client::logging::Logger::getNetworkLogger();

        if (!error) {
            logger->info("TCP socket connected, starting TLS handshake...");

            // Perform async TLS handshake
            _socket.async_handshake(
                ssl::stream_base::client,
                [this](const boost::system::error_code &hsError) {
                    auto logger = client::logging::Logger::getNetworkLogger();
                    if (hsError) {
                        logger->error("TLS handshake failed: {}", hsError.message());
                        _connecting.store(false);
                        _eventQueue.push(TCPErrorEvent{"TLS handshake failed: " + hsError.message()});
                        if (_onError) {
                            _onError("TLS handshake failed: " + hsError.message());
                        }
                        return;
                    }

                    logger->info("TLS handshake successful, waiting for server response...");

                    {
                        std::lock_guard<std::mutex> lock(_heartbeatMutex);
                        _lastServerResponse = std::chrono::steady_clock::now();
                    }

                    // Send initial heartbeat to verify server application is responding
                    sendHeartbeat();
                    scheduleHeartbeat();

                    // Start reading - TCPConnectedEvent will be pushed when HeartBeatAck is received
                    asyncRead();
                }
            );
        } else {
            logger->error("Connection failed: {}", error.message());

            _connecting.store(false);  // Reset connecting state on failure

            _eventQueue.push(TCPErrorEvent{"Connexion echouee: " + error.message()});
            if (_onError) {
                _onError("Connexion echouee: " + error.message());
            }
        }
    }

    void TCPClient::handleRead(const boost::system::error_code &error, std::size_t bytes)
    {
        // Ignore operation_aborted - expected when intentionally disconnecting
        if (error == boost::asio::error::operation_aborted) {
            return;
        }

        auto logger = client::logging::Logger::getNetworkLogger();

        if (!error) {
            {
                std::lock_guard<std::mutex> lock(_heartbeatMutex);
                _lastServerResponse = std::chrono::steady_clock::now();
            }

            _accumulator.insert(_accumulator.end(), _readBuffer, _readBuffer + bytes);

            while (_accumulator.size() >= Header::WIRE_SIZE) {
                auto headOpt = Header::from_bytes(_accumulator.data(), _accumulator.size());
                if (!headOpt) {
                    break;
                }
                Header head = *headOpt;
                size_t totalSize = Header::WIRE_SIZE + head.payload_size;

                if (_accumulator.size() < totalSize) {
                    break;
                }

                _isAuthenticated = head.isAuthenticated;

                if (head.type == static_cast<uint16_t>(MessageType::Login)) {
                    // Server prompts for login - send credentials
                    std::scoped_lock lock(_mutex);
                    if (!_pendingUsername.empty() && !_pendingPassword.empty()) {
                        sendLoginData(_pendingUsername, _pendingPassword);
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::Register)) {
                    // Server prompts for register - send credentials
                    std::scoped_lock lock(_mutex);
                    if (!_pendingUsername.empty() && !_pendingEmail.empty() && !_pendingPassword.empty()) {
                        sendRegisterData(_pendingUsername, _pendingEmail, _pendingPassword);
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::LoginAck) ||
                         head.type == static_cast<uint16_t>(MessageType::RegisterAck)) {
                    // Server response to login/register - try AuthResponseWithToken first
                    if (head.payload_size >= AuthResponseWithToken::WIRE_SIZE) {
                        auto respOpt = AuthResponseWithToken::from_bytes(
                            _accumulator.data() + Header::WIRE_SIZE,
                            head.payload_size
                        );
                        if (respOpt) {
                            if (respOpt->success) {
                                _isAuthenticated = true;
                                // Store the session token and user email for UDP JoinGame and friend system
                                {
                                    std::scoped_lock lock(_mutex);
                                    _sessionToken = respOpt->token;
                                    // Store user email from server response (critical for friend system identification)
                                    _pendingEmail = std::string(respOpt->userEmail);
                                    logger->debug("Stored user email from server: {}", _pendingEmail);
                                }

                                // Check version compatibility
                                auto clientVersion = client::core::getClientVersion();
                                if (!client::core::isVersionCompatible(clientVersion, respOpt->serverVersion)) {
                                    // Version mismatch - calculate how many commits behind
                                    int commitsBehind = respOpt->versionHistory.findPosition(clientVersion.gitHash);
                                    logger->warn("Version mismatch! Client: {} Server: {} ({} commits behind)",
                                        client::core::formatVersion(clientVersion),
                                        client::core::formatVersion(respOpt->serverVersion),
                                        commitsBehind >= 0 ? std::to_string(commitsBehind) : "unknown");
                                    _eventQueue.push(TCPVersionMismatchEvent{clientVersion, respOpt->serverVersion, commitsBehind});
                                } else {
                                    logger->info("Version compatible: {}", client::core::formatVersion(clientVersion));
                                }

                                logger->info("Authentication successful, token received");
                                _eventQueue.push(TCPAuthSuccessEvent{respOpt->serverVersion});
                                if (_onReceive) {
                                    _onReceive("authenticated");
                                }
                            } else {
                                logger->warn("Authentication failed: {} - {}", respOpt->error_code, respOpt->message);
                                _eventQueue.push(TCPAuthFailedEvent{std::string(respOpt->message)});
                                if (_onError) {
                                    _onError(std::string(respOpt->message));
                                }
                            }
                        }
                    }
                    // Fallback to AuthResponse (without token) for backward compatibility
                    else if (head.payload_size >= AuthResponse::WIRE_SIZE) {
                        auto respOpt = AuthResponse::from_bytes(
                            _accumulator.data() + Header::WIRE_SIZE,
                            head.payload_size
                        );
                        if (respOpt) {
                            if (respOpt->success) {
                                _isAuthenticated = true;
                                logger->info("Authentication successful (no token, old server)");
                                // Old server without version info - create empty version
                                VersionInfo emptyVersion{};
                                _eventQueue.push(TCPAuthSuccessEvent{emptyVersion});
                                if (_onReceive) {
                                    _onReceive("authenticated");
                                }
                            } else {
                                logger->warn("Authentication failed: {} - {}", respOpt->error_code, respOpt->message);
                                _eventQueue.push(TCPAuthFailedEvent{std::string(respOpt->message)});
                                if (_onError) {
                                    _onError(std::string(respOpt->message));
                                }
                            }
                        }
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::HeartBeatAck)) {
                    // Confirm connection on first HeartBeatAck
                    if (_connecting.exchange(false)) {
                        _connected.store(true);
                        logger->info("TCP connection confirmed by server");
                        _eventQueue.push(TCPConnectedEvent{});
                        if (_onConnected) {
                            _onConnected();
                        }
                    }
                }
                // Room messages
                else if (head.type == static_cast<uint16_t>(MessageType::CreateRoomAck)) {
                    auto ackOpt = CreateRoomAck::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (ackOpt) {
                        if (ackOpt->success) {
                            std::string code(ackOpt->roomCode, ROOM_CODE_LEN);
                            {
                                std::scoped_lock lock(_mutex);
                                _currentRoomCode = code;
                                _isHost = true;
                                _isReady = false;
                            }
                            logger->info("Room created with code: {}", code);
                            _eventQueue.push(TCPRoomCreatedEvent{code});
                        } else {
                            logger->warn("Room creation failed: {}", ackOpt->message);
                            _eventQueue.push(TCPRoomCreateFailedEvent{
                                std::string(ackOpt->errorCode),
                                std::string(ackOpt->message)
                            });
                        }
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::JoinRoomAck)) {
                    auto ackOpt = JoinRoomAck::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (ackOpt) {
                        std::string code(ackOpt->roomCode, ROOM_CODE_LEN);
                        std::string name(ackOpt->roomName);
                        {
                            std::scoped_lock lock(_mutex);
                            _currentRoomCode = code;
                            _roomSlotId = ackOpt->slotId;
                            _isHost = (ackOpt->isHost != 0);
                            _isReady = false;
                        }

                        auto players = extractPlayerList(*ackOpt);
                        logger->info("Joined room '{}' (code: {}, slot: {}, players: {})",
                                    name, code, ackOpt->slotId, players.size());
                        _eventQueue.push(TCPRoomJoinedEvent{
                            ackOpt->slotId, name, code, ackOpt->maxPlayers, ackOpt->isHost != 0,
                            std::move(players)
                        });
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::JoinRoomNack)) {
                    auto nackOpt = JoinRoomNack::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (nackOpt) {
                        logger->warn("Failed to join room: {}", nackOpt->message);
                        _eventQueue.push(TCPRoomJoinFailedEvent{
                            std::string(nackOpt->errorCode),
                            std::string(nackOpt->message)
                        });
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::LeaveRoomAck)) {
                    {
                        std::scoped_lock lock(_mutex);
                        _currentRoomCode.reset();
                        _isHost = false;
                        _isReady = false;
                        _roomSlotId = 0;
                    }
                    logger->info("Left room");
                    _eventQueue.push(TCPRoomLeftEvent{});
                }
                else if (head.type == static_cast<uint16_t>(MessageType::SetReadyAck)) {
                    auto ackOpt = SetReadyAck::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (ackOpt) {
                        bool ready = (ackOpt->isReady != 0);
                        {
                            std::scoped_lock lock(_mutex);
                            _isReady = ready;
                        }
                        logger->debug("Ready status changed to: {}", ready);
                        _eventQueue.push(TCPReadyChangedEvent{ready});
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::StartGameAck)) {
                    logger->info("Game start confirmed");
                    _eventQueue.push(TCPStartGameAckEvent{});
                }
                else if (head.type == static_cast<uint16_t>(MessageType::StartGameNack)) {
                    auto nackOpt = StartGameNack::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (nackOpt) {
                        logger->warn("Cannot start game: {}", nackOpt->message);
                        // Could push an event here if needed
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::RoomUpdate)) {
                    auto updateOpt = RoomUpdate::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (updateOpt) {
                        TCPRoomUpdateEvent event;
                        event.roomName = std::string(updateOpt->roomName);
                        event.roomCode = std::string(updateOpt->roomCode, ROOM_CODE_LEN);
                        event.maxPlayers = updateOpt->maxPlayers;
                        event.gameSpeedPercent = updateOpt->gameSpeedPercent;
                        for (uint8_t i = 0; i < updateOpt->playerCount; ++i) {
                            const auto& ps = updateOpt->players[i];
                            event.players.push_back(RoomPlayerInfo{
                                ps.slotId,
                                std::string(ps.displayName),
                                std::string(ps.email),
                                ps.isReady != 0,
                                ps.isHost != 0,
                                ps.shipSkin
                            });
                        }
                        logger->debug("Room update: {} players", event.players.size());
                        _eventQueue.push(std::move(event));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::GameStarting)) {
                    auto gsOpt = GameStarting::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (gsOpt) {
                        logger->info("Game starting! Countdown: {}", gsOpt->countdownSeconds);
                        _eventQueue.push(TCPGameStartingEvent{gsOpt->countdownSeconds});
                    }
                }
                // Kick messages (Phase 2)
                else if (head.type == static_cast<uint16_t>(MessageType::KickPlayerAck)) {
                    logger->info("Kick player acknowledged");
                    _eventQueue.push(TCPKickSuccessEvent{});
                }
                else if (head.type == static_cast<uint16_t>(MessageType::PlayerKicked)) {
                    auto notifOpt = PlayerKickedNotification::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (notifOpt) {
                        std::string reason(notifOpt->reason);
                        logger->warn("You were kicked from the room! Reason: {}", reason.empty() ? "none" : reason);
                        {
                            std::scoped_lock lock(_mutex);
                            _currentRoomCode.reset();
                            _isHost = false;
                            _isReady = false;
                            _roomSlotId = 0;
                        }
                        _eventQueue.push(TCPPlayerKickedEvent{reason});
                    }
                }
                // Room Browser messages (Phase 2)
                else if (head.type == static_cast<uint16_t>(MessageType::BrowsePublicRoomsAck)) {
                    auto respOpt = BrowsePublicRoomsResponse::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (respOpt) {
                        TCPRoomListEvent event;
                        for (uint8_t i = 0; i < respOpt->roomCount; ++i) {
                            const auto& entry = respOpt->rooms[i];
                            event.rooms.push_back(RoomBrowserInfo{
                                std::string(entry.code, ROOM_CODE_LEN),
                                std::string(entry.name),
                                entry.currentPlayers,
                                entry.maxPlayers
                            });
                        }
                        logger->debug("Received {} public rooms", event.rooms.size());
                        _eventQueue.push(std::move(event));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::QuickJoinAck)) {
                    // Same format as JoinRoomAck
                    auto ackOpt = JoinRoomAck::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (ackOpt) {
                        std::string code(ackOpt->roomCode, ROOM_CODE_LEN);
                        std::string name(ackOpt->roomName);
                        {
                            std::scoped_lock lock(_mutex);
                            _currentRoomCode = code;
                            _roomSlotId = ackOpt->slotId;
                            _isHost = (ackOpt->isHost != 0);
                            _isReady = false;
                        }

                        auto players = extractPlayerList(*ackOpt);
                        logger->info("Quick joined room '{}' (code: {}, slot: {}, players: {})",
                                    name, code, ackOpt->slotId, players.size());
                        _eventQueue.push(TCPRoomJoinedEvent{
                            ackOpt->slotId, name, code, ackOpt->maxPlayers, ackOpt->isHost != 0,
                            std::move(players)
                        });
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::QuickJoinNack)) {
                    auto nackOpt = QuickJoinNack::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (nackOpt) {
                        logger->warn("Quick join failed: {}", nackOpt->message);
                        _eventQueue.push(TCPQuickJoinFailedEvent{
                            std::string(nackOpt->errorCode),
                            std::string(nackOpt->message)
                        });
                    }
                }
                // User Settings messages (Phase 2)
                else if (head.type == static_cast<uint16_t>(MessageType::GetUserSettingsAck)) {
                    auto respOpt = GetUserSettingsResponse::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (respOpt) {
                        TCPUserSettingsEvent event;
                        event.found = (respOpt->found != 0);
                        event.colorBlindMode = std::string(respOpt->settings.colorBlindMode);
                        event.gameSpeed = static_cast<float>(respOpt->settings.gameSpeedPercent) / 100.0f;
                        std::memcpy(event.keyBindings.data(), respOpt->settings.keyBindings, KEY_BINDINGS_COUNT);
                        event.shipSkin = respOpt->settings.shipSkin;
                        // Voice settings
                        event.voiceMode = respOpt->settings.voiceMode;
                        event.vadThreshold = respOpt->settings.vadThreshold;
                        event.micGain = respOpt->settings.micGain;
                        event.voiceVolume = respOpt->settings.voiceVolume;
                        // Audio device names
                        event.audioInputDevice = std::string(respOpt->settings.audioInputDevice);
                        event.audioOutputDevice = std::string(respOpt->settings.audioOutputDevice);
                        // Chat settings
                        event.keepChatOpenAfterSend = (respOpt->settings.keepChatOpenAfterSend != 0);
                        logger->debug("Received user settings (found={}, input='{}', output='{}', keepChatOpen={})",
                            event.found, event.audioInputDevice, event.audioOutputDevice, event.keepChatOpenAfterSend);
                        _eventQueue.push(std::move(event));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::SaveUserSettingsAck)) {
                    auto respOpt = SaveUserSettingsResponse::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (respOpt) {
                        logger->debug("Save settings result: success={}", respOpt->success);
                        _eventQueue.push(TCPSaveSettingsResultEvent{
                            respOpt->success != 0,
                            std::string(respOpt->message)
                        });
                    }
                }
                // Chat System (Phase 2)
                else if (head.type == static_cast<uint16_t>(MessageType::ChatMessageBroadcast)) {
                    auto msgOpt = ChatMessagePayload::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (msgOpt) {
                        logger->debug("Chat message from {}: {}", msgOpt->displayName, msgOpt->message);
                        _eventQueue.push(TCPChatMessageEvent{
                            std::string(msgOpt->displayName),
                            std::string(msgOpt->message),
                            msgOpt->timestamp
                        });
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::ChatHistory)) {
                    auto histOpt = ChatHistoryResponse::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (histOpt) {
                        logger->debug("Received chat history with {} messages", histOpt->messageCount);
                        TCPChatHistoryEvent event;
                        for (uint8_t i = 0; i < histOpt->messageCount; ++i) {
                            event.messages.push_back(ChatMessageInfo{
                                std::string(histOpt->messages[i].displayName),
                                std::string(histOpt->messages[i].message),
                                histOpt->messages[i].timestamp
                            });
                        }
                        _eventQueue.push(std::move(event));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::SendChatMessageAck)) {
                    logger->debug("Chat message sent successfully");
                    // No event needed - just confirmation
                }
                // Leaderboard System (Phase 3)
                else if (head.type == static_cast<uint16_t>(MessageType::LeaderboardData)) {
                    auto respOpt = LeaderboardDataResponse::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (respOpt) {
                        logger->debug("Received leaderboard data: {} entries, yourRank={}", respOpt->count, respOpt->yourRank);
                        // Parse entries from the payload
                        LeaderboardDataEvent evt;
                        evt.response = *respOpt;
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE + LeaderboardDataResponse::HEADER_SIZE;
                        for (uint8_t i = 0; i < respOpt->count; ++i) {
                            auto entryOpt = LeaderboardEntryWire::from_bytes(ptr, LeaderboardEntryWire::WIRE_SIZE);
                            if (entryOpt) {
                                evt.entries.push_back(*entryOpt);
                            }
                            ptr += LeaderboardEntryWire::WIRE_SIZE;
                        }
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::PlayerStatsData)) {
                    auto statsOpt = PlayerStatsWire::from_bytes(
                        _accumulator.data() + Header::WIRE_SIZE, head.payload_size);
                    if (statsOpt) {
                        logger->debug("Received player stats for {}", statsOpt->playerName);
                        _eventQueue.push(PlayerStatsDataEvent{*statsOpt});
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::AchievementsData)) {
                    if (head.payload_size >= 4) {
                        uint32_t netBitfield;
                        std::memcpy(&netBitfield, _accumulator.data() + Header::WIRE_SIZE, 4);
                        uint32_t bitfield = swap32(netBitfield);
                        logger->debug("Received achievements bitfield: 0x{:08X}", bitfield);
                        _eventQueue.push(AchievementsDataEvent{bitfield});
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::GameHistoryData)) {
                    if (head.payload_size >= 1) {
                        uint8_t count = _accumulator[Header::WIRE_SIZE];
                        GameHistoryDataEvent evt;
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE + 1;
                        for (uint8_t i = 0; i < count && (1 + i * GameHistoryEntryWire::WIRE_SIZE) <= head.payload_size; ++i) {
                            auto entryOpt = GameHistoryEntryWire::from_bytes(ptr, GameHistoryEntryWire::WIRE_SIZE);
                            if (entryOpt) {
                                evt.entries.push_back(*entryOpt);
                            }
                            ptr += GameHistoryEntryWire::WIRE_SIZE;
                        }
                        logger->debug("Received {} game history entries", evt.entries.size());
                        _eventQueue.push(std::move(evt));
                    }
                }
                // ═══════════════════════════════════════════════════════════════════
                // Friends System (Phase 4)
                // ═══════════════════════════════════════════════════════════════════
                else if (head.type == static_cast<uint16_t>(MessageType::SendFriendRequestAck)) {
                    if (head.payload_size >= 1 + MAX_EMAIL_LEN) {
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE;
                        TCPFriendRequestAckEvent evt;
                        evt.errorCode = ptr[0];
                        evt.targetEmail = std::string(reinterpret_cast<const char*>(ptr + 1), MAX_EMAIL_LEN);
                        evt.targetEmail = evt.targetEmail.c_str();  // Trim at null
                        logger->debug("Friend request ack: error={}", evt.errorCode);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::FriendRequestReceived)) {
                    if (head.payload_size >= MAX_EMAIL_LEN + PLAYER_NAME_LEN) {
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE;
                        TCPFriendRequestReceivedEvent evt;
                        evt.fromEmail = std::string(reinterpret_cast<const char*>(ptr), MAX_EMAIL_LEN);
                        evt.fromEmail = evt.fromEmail.c_str();
                        evt.fromDisplayName = std::string(reinterpret_cast<const char*>(ptr + MAX_EMAIL_LEN), PLAYER_NAME_LEN);
                        evt.fromDisplayName = evt.fromDisplayName.c_str();
                        logger->info("Friend request received from: {}", evt.fromDisplayName);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::AcceptFriendRequestAck)) {
                    if (head.payload_size >= 1) {
                        TCPAcceptFriendRequestAckEvent evt;
                        evt.errorCode = _accumulator[Header::WIRE_SIZE];
                        logger->debug("Accept friend request ack: error={}", evt.errorCode);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::FriendRequestAccepted)) {
                    if (head.payload_size >= MAX_EMAIL_LEN + PLAYER_NAME_LEN + 1) {
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE;
                        TCPFriendRequestAcceptedEvent evt;
                        evt.friendEmail = std::string(reinterpret_cast<const char*>(ptr), MAX_EMAIL_LEN);
                        evt.friendEmail = evt.friendEmail.c_str();
                        evt.displayName = std::string(reinterpret_cast<const char*>(ptr + MAX_EMAIL_LEN), PLAYER_NAME_LEN);
                        evt.displayName = evt.displayName.c_str();
                        evt.onlineStatus = ptr[MAX_EMAIL_LEN + PLAYER_NAME_LEN];
                        logger->info("Friend request accepted: {} is now your friend!", evt.displayName);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::RejectFriendRequestAck)) {
                    if (head.payload_size >= 1) {
                        TCPRejectFriendRequestAckEvent evt;
                        evt.errorCode = _accumulator[Header::WIRE_SIZE];
                        logger->debug("Reject friend request ack: error={}", evt.errorCode);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::RemoveFriendAck)) {
                    if (head.payload_size >= 1) {
                        TCPRemoveFriendAckEvent evt;
                        evt.errorCode = _accumulator[Header::WIRE_SIZE];
                        logger->debug("Remove friend ack: error={}", evt.errorCode);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::FriendRemoved)) {
                    if (head.payload_size >= MAX_EMAIL_LEN) {
                        TCPFriendRemovedEvent evt;
                        evt.friendEmail = std::string(reinterpret_cast<const char*>(_accumulator.data() + Header::WIRE_SIZE), MAX_EMAIL_LEN);
                        evt.friendEmail = evt.friendEmail.c_str();
                        logger->info("Friend removed: {}", evt.friendEmail);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::BlockUserAck)) {
                    if (head.payload_size >= 1) {
                        TCPBlockUserAckEvent evt;
                        evt.errorCode = _accumulator[Header::WIRE_SIZE];
                        logger->debug("Block user ack: error={}", evt.errorCode);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::UnblockUserAck)) {
                    if (head.payload_size >= 1) {
                        TCPUnblockUserAckEvent evt;
                        evt.errorCode = _accumulator[Header::WIRE_SIZE];
                        logger->debug("Unblock user ack: error={}", evt.errorCode);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::FriendsListData)) {
                    if (head.payload_size >= 2) {
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE;
                        uint8_t count = ptr[0];
                        uint8_t totalCount = ptr[1];
                        ptr += 2;

                        TCPFriendsListEvent evt;
                        evt.totalCount = totalCount;
                        for (uint8_t i = 0; i < count; ++i) {
                            auto friendOpt = FriendInfoWire::from_bytes(ptr, FriendInfoWire::WIRE_SIZE);
                            if (friendOpt) {
                                FriendInfo info;
                                info.email = std::string(friendOpt->email);
                                info.displayName = std::string(friendOpt->displayName);
                                info.onlineStatus = friendOpt->onlineStatus;
                                info.currentRoomCode = std::string(friendOpt->roomCode);
                                evt.friends.push_back(std::move(info));
                            }
                            ptr += FriendInfoWire::WIRE_SIZE;
                        }
                        logger->debug("Received {} friends (total: {})", evt.friends.size(), evt.totalCount);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::FriendRequestsData)) {
                    if (head.payload_size >= 2) {
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE;
                        uint8_t incomingCount = ptr[0];
                        uint8_t outgoingCount = ptr[1];
                        ptr += 2;

                        TCPFriendRequestsEvent evt;
                        for (uint8_t i = 0; i < incomingCount; ++i) {
                            auto reqOpt = FriendRequestInfoWire::from_bytes(ptr, FriendRequestInfoWire::WIRE_SIZE);
                            if (reqOpt) {
                                FriendRequestInfo info;
                                info.email = std::string(reqOpt->email);
                                info.displayName = std::string(reqOpt->displayName);
                                info.timestamp = reqOpt->timestamp;
                                evt.incoming.push_back(std::move(info));
                            }
                            ptr += FriendRequestInfoWire::WIRE_SIZE;
                        }
                        for (uint8_t i = 0; i < outgoingCount; ++i) {
                            auto reqOpt = FriendRequestInfoWire::from_bytes(ptr, FriendRequestInfoWire::WIRE_SIZE);
                            if (reqOpt) {
                                FriendRequestInfo info;
                                info.email = std::string(reqOpt->email);
                                info.displayName = std::string(reqOpt->displayName);
                                info.timestamp = reqOpt->timestamp;
                                evt.outgoing.push_back(std::move(info));
                            }
                            ptr += FriendRequestInfoWire::WIRE_SIZE;
                        }
                        logger->debug("Received {} incoming, {} outgoing friend requests", evt.incoming.size(), evt.outgoing.size());
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::BlockedUsersData)) {
                    if (head.payload_size >= 1) {
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE;
                        uint8_t count = ptr[0];
                        ptr += 1;

                        TCPBlockedUsersEvent evt;
                        for (uint8_t i = 0; i < count; ++i) {
                            auto friendOpt = FriendInfoWire::from_bytes(ptr, FriendInfoWire::WIRE_SIZE);
                            if (friendOpt) {
                                FriendInfo info;
                                info.email = std::string(friendOpt->email);
                                info.displayName = std::string(friendOpt->displayName);
                                info.onlineStatus = friendOpt->onlineStatus;
                                info.currentRoomCode = std::string(friendOpt->roomCode);
                                evt.blockedUsers.push_back(std::move(info));
                            }
                            ptr += FriendInfoWire::WIRE_SIZE;
                        }
                        logger->debug("Received {} blocked users", evt.blockedUsers.size());
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::FriendStatusChanged)) {
                    if (head.payload_size >= MAX_EMAIL_LEN + 1 + ROOM_CODE_LEN) {
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE;
                        TCPFriendStatusChangedEvent evt;
                        evt.friendEmail = std::string(reinterpret_cast<const char*>(ptr), MAX_EMAIL_LEN);
                        evt.friendEmail = evt.friendEmail.c_str();
                        evt.newStatus = ptr[MAX_EMAIL_LEN];
                        evt.roomCode = std::string(reinterpret_cast<const char*>(ptr + MAX_EMAIL_LEN + 1), ROOM_CODE_LEN);
                        evt.roomCode = evt.roomCode.c_str();
                        logger->debug("Friend {} status changed to {}", evt.friendEmail, evt.newStatus);
                        _eventQueue.push(std::move(evt));
                    }
                }
                // ═══════════════════════════════════════════════════════════════════
                // Private Messaging (Phase 4)
                // ═══════════════════════════════════════════════════════════════════
                else if (head.type == static_cast<uint16_t>(MessageType::SendPrivateMessageAck)) {
                    if (head.payload_size >= 1 + 8) {
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE;
                        TCPPrivateMessageAckEvent evt;
                        evt.errorCode = ptr[0];
                        uint64_t netId;
                        std::memcpy(&netId, ptr + 1, 8);
                        evt.messageId = swap64(netId);
                        logger->debug("Private message ack: error={}, id={}", evt.errorCode, evt.messageId);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::PrivateMessageReceived)) {
                    if (head.payload_size >= MAX_EMAIL_LEN + PLAYER_NAME_LEN + 8 + 2) {
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE;
                        TCPPrivateMessageReceivedEvent evt;
                        evt.senderEmail = std::string(reinterpret_cast<const char*>(ptr), MAX_EMAIL_LEN);
                        evt.senderEmail = evt.senderEmail.c_str();
                        ptr += MAX_EMAIL_LEN;
                        evt.senderDisplayName = std::string(reinterpret_cast<const char*>(ptr), PLAYER_NAME_LEN);
                        evt.senderDisplayName = evt.senderDisplayName.c_str();
                        ptr += PLAYER_NAME_LEN;

                        uint64_t netTs;
                        std::memcpy(&netTs, ptr, 8);
                        evt.timestamp = swap64(netTs);
                        ptr += 8;

                        uint16_t netLen;
                        std::memcpy(&netLen, ptr, 2);
                        uint16_t msgLen = swap16(netLen);
                        ptr += 2;

                        evt.message = std::string(reinterpret_cast<const char*>(ptr), msgLen);
                        logger->info("Private message from {}: {}", evt.senderDisplayName, evt.message.substr(0, 50));
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::ConversationData)) {
                    if (head.payload_size >= 2) {
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE;
                        uint8_t count = ptr[0];
                        bool hasMore = ptr[1] != 0;
                        ptr += 2;

                        TCPConversationEvent evt;
                        evt.hasMore = hasMore;
                        for (uint8_t i = 0; i < count; ++i) {
                            auto msgOpt = PrivateMessageWire::from_bytes(ptr, PrivateMessageWire::WIRE_SIZE);
                            if (msgOpt) {
                                PrivateMessageInfo info;
                                info.messageId = msgOpt->timestamp;  // Use timestamp as unique ID
                                info.senderEmail = std::string(msgOpt->senderEmail);
                                info.senderDisplayName = std::string(msgOpt->senderDisplayName);
                                info.timestamp = msgOpt->timestamp;
                                info.isRead = (msgOpt->isRead != 0);
                                info.message = std::string(msgOpt->message);
                                evt.messages.push_back(std::move(info));
                                ptr += PrivateMessageWire::WIRE_SIZE;
                            } else {
                                break;
                            }
                        }
                        logger->debug("Received {} messages, hasMore={}", evt.messages.size(), evt.hasMore);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::ConversationsListData)) {
                    if (head.payload_size >= 1) {
                        const uint8_t* ptr = _accumulator.data() + Header::WIRE_SIZE;
                        uint8_t count = ptr[0];
                        ptr += 1;

                        TCPConversationsListEvent evt;
                        for (uint8_t i = 0; i < count; ++i) {
                            auto convOpt = ConversationSummaryWire::from_bytes(ptr, ConversationSummaryWire::WIRE_SIZE);
                            if (convOpt) {
                                ConversationSummary info;
                                info.otherEmail = std::string(convOpt->otherEmail);
                                info.otherDisplayName = std::string(convOpt->otherDisplayName);
                                info.lastMessagePreview = std::string(convOpt->lastMessage);
                                info.lastMessageTimestamp = convOpt->lastTimestamp;
                                info.unreadCount = convOpt->unreadCount;
                                evt.conversations.push_back(std::move(info));
                                ptr += ConversationSummaryWire::WIRE_SIZE;
                            } else {
                                break;
                            }
                        }
                        logger->debug("Received {} conversations", evt.conversations.size());
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::MarkMessagesReadAck)) {
                    if (head.payload_size >= 1) {
                        TCPMarkMessagesReadAckEvent evt;
                        evt.errorCode = _accumulator[Header::WIRE_SIZE];
                        logger->debug("Mark messages read ack: error={}", evt.errorCode);
                        _eventQueue.push(std::move(evt));
                    }
                }
                else if (head.type == static_cast<uint16_t>(MessageType::MessagesReadNotification)) {
                    if (head.payload_size >= MAX_EMAIL_LEN) {
                        TCPMessagesReadNotificationEvent evt;
                        const char* ptr = reinterpret_cast<const char*>(_accumulator.data() + Header::WIRE_SIZE);
                        evt.readerEmail = std::string(ptr, strnlen(ptr, MAX_EMAIL_LEN));
                        logger->debug("Messages read notification: {} read our messages", evt.readerEmail);
                        _eventQueue.push(std::move(evt));
                    }
                }

                _accumulator.erase(_accumulator.begin(), _accumulator.begin() + totalSize);
            }

            asyncRead();
        } else {
            if (error == boost::asio::error::eof) {
                logger->info("Server disconnected");
            } else {
                logger->error("Read error: {}", error.message());
            }

            _eventQueue.push(TCPErrorEvent{"Erreur lecture: " + error.message()});
            if (_onError) {
                _onError("Erreur lecture: " + error.message());
            }

            disconnect();
        }
    }

    void TCPClient::setLoginCredentials(const std::string& username, const std::string& password) {
        std::scoped_lock lock(_mutex);
        _pendingUsername = username;
        _pendingPassword = password;
    }

    void TCPClient::setRegisterCredentials(const std::string& username, const std::string& email, const std::string& password) {
        std::scoped_lock lock(_mutex);
        _pendingUsername = username;
        _pendingEmail = email;
        _pendingPassword = password;
    }

    void TCPClient::sendLoginData(const std::string& username, const std::string& password) {
        // Store credentials for potential auto-reconnection
        {
            std::scoped_lock lock(_mutex);
            _storedUsername = username;
            _storedPassword = password;
            // If username contains '@', it's likely an email - store it for friend system
            if (username.find('@') != std::string::npos) {
                _pendingEmail = username;
            }
        }

        LoginMessage login;
        std::snprintf(login.username, sizeof(login.username), "%s", username.c_str());
        std::snprintf(login.password, sizeof(login.password), "%s", password.c_str());

        Header head = {.isAuthenticated = false, .type = static_cast<uint16_t>(MessageType::Login), .payload_size = sizeof(login)};

        const size_t totalSize = Header::WIRE_SIZE + sizeof(login);

        auto buf = std::make_shared<std::vector<uint8_t>>(totalSize);

        head.to_bytes(buf->data());
        login.to_bytes(buf->data() + Header::WIRE_SIZE);

        boost::asio::async_write(
            _socket,
            boost::asio::buffer(buf->data(), totalSize),
            [this, buf](const boost::system::error_code &error, std::size_t) {
                if (error && _onError) {
                    _onError("Write error: " + error.message());
                }
            }
        );
    }

    void TCPClient::sendRegisterData(const std::string& username, const std::string& email, const std::string& password) {
        RegisterMessage registerUser;
        std::snprintf(registerUser.username, sizeof(registerUser.username), "%s", username.c_str());
        std::snprintf(registerUser.email, sizeof(registerUser.email), "%s", email.c_str());
        std::snprintf(registerUser.password, sizeof(registerUser.password), "%s", password.c_str());

        Header head = {.isAuthenticated = false, .type = static_cast<uint16_t>(MessageType::Register), .payload_size = sizeof(registerUser)};

        const size_t totalSize = Header::WIRE_SIZE + sizeof(registerUser);

        auto buf = std::make_shared<std::vector<uint8_t>>(totalSize);

        head.to_bytes(buf->data());
        registerUser.to_bytes(buf->data() + Header::WIRE_SIZE);

        boost::asio::async_write(
            _socket,
            boost::asio::buffer(buf->data(), totalSize),
            [this, buf](const boost::system::error_code &error, std::size_t) {
                if (error && _onError) {
                    _onError("Write error: " + error.message());
                }
            }
        );
    }

    bool TCPClient::isAuthenticated() const {
        return _isAuthenticated;
    }

    void TCPClient::sendHeartbeat() {
        // Check socket is open (works for both connected and connecting states)
        if (!_socket.lowest_layer().is_open()) {
            return;
        }

        Header head{
            .isAuthenticated = _isAuthenticated.load(),
            .type = static_cast<uint16_t>(MessageType::HeartBeat),
            .payload_size = 0
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE);
        head.to_bytes(buf->data());

        boost::asio::async_write(
            _socket,
            boost::asio::buffer(buf->data(), Header::WIRE_SIZE),
            [buf](const boost::system::error_code &error, std::size_t) {
                if (error) {
                    client::logging::Logger::getNetworkLogger()->error("HeartBeat write error: {}", error.message());
                }
            }
        );
    }

    void TCPClient::scheduleHeartbeat() {
        _heartbeatTimer.expires_after(std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS));
        _heartbeatTimer.async_wait([this](boost::system::error_code ec) {
            // Continue if connected OR connecting (waiting for HeartBeatAck)
            if (ec || (!_connected.load() && !_connecting.load())) {
                return;
            }

            std::chrono::steady_clock::time_point lastResponse;
            {
                std::lock_guard<std::mutex> lock(_heartbeatMutex);
                lastResponse = _lastServerResponse;
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastResponse
            ).count();

            if (elapsed > HEARTBEAT_TIMEOUT_MS) {
                auto logger = client::logging::Logger::getNetworkLogger();

                if (_connecting.load()) {
                    logger->warn("Initial TCP connection timeout ({}ms) - server not responding", elapsed);
                    _eventQueue.push(TCPErrorEvent{"Timeout: Serveur TCP injoignable"});
                } else {
                    logger->warn("TCP Server heartbeat timeout ({}ms)", elapsed);
                    _eventQueue.push(TCPErrorEvent{"Timeout: Serveur d'authentification injoignable"});
                }

                if (_onError) {
                    _onError("Timeout: Serveur d'authentification injoignable");
                }

                disconnect();
                return;
            }

            sendHeartbeat();
            scheduleHeartbeat();
        });
    }

    std::optional<TCPEvent> TCPClient::pollEvent() {
        return _eventQueue.poll();
    }

    void TCPClient::clearEventQueue() {
        _eventQueue.clear();
    }

    // ═══════════════════════════════════════════════════════════════════
    // Generic message sending (reduces code duplication)
    // ═══════════════════════════════════════════════════════════════════

    void TCPClient::sendMessageNoPayload(MessageType type, const char* logName) {
        if (!_connected.load() || !_isAuthenticated.load()) {
            return;
        }

        Header head = {
            .isAuthenticated = true,
            .type = static_cast<uint16_t>(type),
            .payload_size = 0
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE);
        head.to_bytes(buf->data());

        boost::asio::async_write(
            _socket,
            boost::asio::buffer(*buf),
            [buf, logName](const boost::system::error_code &error, std::size_t) {
                if (error) {
                    client::logging::Logger::getNetworkLogger()->error("{} write error: {}", logName, error.message());
                }
            }
        );
    }

    template<typename T>
    void TCPClient::sendMessageWithPayload(MessageType type, const T& payload, const char* logName) {
        if (!_connected.load() || !_isAuthenticated.load()) {
            return;
        }

        Header head = {
            .isAuthenticated = true,
            .type = static_cast<uint16_t>(type),
            .payload_size = static_cast<uint32_t>(T::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + T::WIRE_SIZE);
        head.to_bytes(buf->data());
        payload.to_bytes(buf->data() + Header::WIRE_SIZE);

        boost::asio::async_write(
            _socket,
            boost::asio::buffer(*buf),
            [buf, logName](const boost::system::error_code &error, std::size_t) {
                if (error) {
                    client::logging::Logger::getNetworkLogger()->error("{} write error: {}", logName, error.message());
                }
            }
        );
    }

    std::vector<RoomPlayerInfo> TCPClient::extractPlayerList(const JoinRoomAck& ack) {
        std::vector<RoomPlayerInfo> players;
        players.reserve(ack.playerCount);
        for (uint8_t i = 0; i < ack.playerCount; ++i) {
            const auto& ps = ack.players[i];
            players.push_back(RoomPlayerInfo{
                ps.slotId,
                std::string(ps.displayName),
                std::string(ps.email),
                ps.isReady != 0,
                ps.isHost != 0,
                ps.shipSkin
            });
        }
        return players;
    }

    // ═══════════════════════════════════════════════════════════════════
    // Stored credentials for auto-reconnection
    // ═══════════════════════════════════════════════════════════════════

    bool TCPClient::hasStoredCredentials() const {
        std::scoped_lock lock(_mutex);
        return !_storedUsername.empty() && !_storedPassword.empty();
    }

    std::pair<std::string, std::string> TCPClient::getStoredCredentials() const {
        std::scoped_lock lock(_mutex);
        return {_storedUsername, _storedPassword};
    }

    void TCPClient::clearStoredCredentials() {
        std::scoped_lock lock(_mutex);
        _storedUsername.clear();
        _storedPassword.clear();
        _sessionToken.reset();
        _isAuthenticated.store(false);
    }

    // ═══════════════════════════════════════════════════════════════════
    // Session token management
    // ═══════════════════════════════════════════════════════════════════

    void TCPClient::setSessionToken(const SessionToken& token) {
        std::scoped_lock lock(_mutex);
        _sessionToken = token;
    }

    std::optional<SessionToken> TCPClient::getSessionToken() const {
        std::scoped_lock lock(_mutex);
        return _sessionToken;
    }

    bool TCPClient::hasSessionToken() const {
        std::scoped_lock lock(_mutex);
        return _sessionToken.has_value();
    }

    // ═══════════════════════════════════════════════════════════════════
    // Room operations
    // ═══════════════════════════════════════════════════════════════════

    void TCPClient::createRoom(const std::string& name, uint8_t maxPlayers, bool isPrivate) {
        CreateRoomRequest req;
        std::snprintf(req.name, ROOM_NAME_LEN, "%s", name.c_str());
        req.maxPlayers = maxPlayers;
        req.isPrivate = isPrivate ? 1 : 0;
        sendMessageWithPayload(MessageType::CreateRoom, req, "CreateRoom");
    }

    void TCPClient::joinRoomByCode(const std::string& code) {
        JoinRoomByCodeRequest req;
        std::memcpy(req.roomCode, code.c_str(), std::min(code.size(), ROOM_CODE_LEN));
        sendMessageWithPayload(MessageType::JoinRoomByCode, req, "JoinRoomByCode");
    }

    void TCPClient::leaveRoom() {
        sendMessageNoPayload(MessageType::LeaveRoom, "LeaveRoom");
    }

    void TCPClient::setReady(bool ready) {
        SetReadyRequest req;
        req.isReady = ready ? 1 : 0;
        sendMessageWithPayload(MessageType::SetReady, req, "SetReady");
    }

    void TCPClient::startGame() {
        sendMessageNoPayload(MessageType::StartGame, "StartGame");
    }

    void TCPClient::kickPlayer(const std::string& email, const std::string& reason) {
        KickPlayerRequest req;
        std::snprintf(req.email, MAX_EMAIL_LEN, "%s", email.c_str());
        std::snprintf(req.reason, MAX_ERROR_MSG_LEN, "%s", reason.c_str());
        sendMessageWithPayload(MessageType::KickPlayer, req, "KickPlayer");
    }

    void TCPClient::setRoomConfig(uint16_t gameSpeedPercent) {
        SetRoomConfigRequest req;
        req.gameSpeedPercent = gameSpeedPercent;
        sendMessageWithPayload(MessageType::SetRoomConfig, req, "SetRoomConfig");
    }

    void TCPClient::browsePublicRooms() {
        sendMessageNoPayload(MessageType::BrowsePublicRooms, "BrowsePublicRooms");
    }

    void TCPClient::quickJoin() {
        sendMessageNoPayload(MessageType::QuickJoin, "QuickJoin");
    }

    // ═══════════════════════════════════════════════════════════════════
    // User Settings operations (Phase 2)
    // ═══════════════════════════════════════════════════════════════════

    void TCPClient::requestUserSettings() {
        sendMessageNoPayload(MessageType::GetUserSettings, "GetUserSettings");
    }

    void TCPClient::saveUserSettings(const UserSettingsPayload& settings) {
        SaveUserSettingsRequest req;
        req.settings = settings;
        sendMessageWithPayload(MessageType::SaveUserSettings, req, "SaveUserSettings");
    }

    // ═══════════════════════════════════════════════════════════════════
    // Chat System (Phase 2)
    // ═══════════════════════════════════════════════════════════════════

    void TCPClient::sendChatMessage(const std::string& message) {
        if (message.empty() || message.length() > CHAT_MESSAGE_LEN - 1) {
            return;
        }

        SendChatMessageRequest req;
        std::snprintf(req.message, CHAT_MESSAGE_LEN, "%s", message.c_str());
        sendMessageWithPayload(MessageType::SendChatMessage, req, "SendChatMessage");
    }

    // ═══════════════════════════════════════════════════════════════════
    // Leaderboard System (Phase 3)
    // ═══════════════════════════════════════════════════════════════════

    void TCPClient::sendGetLeaderboard(const GetLeaderboardRequest& req) {
        sendMessageWithPayload(MessageType::GetLeaderboard, req, "GetLeaderboard");
    }

    void TCPClient::sendGetPlayerStats() {
        sendMessageNoPayload(MessageType::GetPlayerStats, "GetPlayerStats");
    }

    void TCPClient::sendGetAchievements() {
        sendMessageNoPayload(MessageType::GetAchievements, "GetAchievements");
    }

    void TCPClient::sendGetGameHistory() {
        sendMessageNoPayload(MessageType::GetGameHistory, "GetGameHistory");
    }

    // ═══════════════════════════════════════════════════════════════════
    // Friends System (Phase 4)
    // ═══════════════════════════════════════════════════════════════════

    void TCPClient::sendFriendRequest(const std::string& targetEmail) {
        SendFriendRequestPayload payload;
        std::memset(&payload, 0, sizeof(payload));
        std::strncpy(payload.targetEmail, targetEmail.c_str(), MAX_EMAIL_LEN - 1);
        sendMessageWithPayload(MessageType::SendFriendRequest, payload, "SendFriendRequest");
    }

    void TCPClient::acceptFriendRequest(const std::string& fromEmail) {
        AcceptFriendRequestPayload payload;
        std::memset(&payload, 0, sizeof(payload));
        std::strncpy(payload.fromEmail, fromEmail.c_str(), MAX_EMAIL_LEN - 1);
        sendMessageWithPayload(MessageType::AcceptFriendRequest, payload, "AcceptFriendRequest");
    }

    void TCPClient::rejectFriendRequest(const std::string& fromEmail) {
        RejectFriendRequestPayload payload;
        std::memset(&payload, 0, sizeof(payload));
        std::strncpy(payload.fromEmail, fromEmail.c_str(), MAX_EMAIL_LEN - 1);
        sendMessageWithPayload(MessageType::RejectFriendRequest, payload, "RejectFriendRequest");
    }

    void TCPClient::removeFriend(const std::string& friendEmail) {
        RemoveFriendPayload payload;
        std::memset(&payload, 0, sizeof(payload));
        std::strncpy(payload.friendEmail, friendEmail.c_str(), MAX_EMAIL_LEN - 1);
        sendMessageWithPayload(MessageType::RemoveFriend, payload, "RemoveFriend");
    }

    void TCPClient::blockUser(const std::string& targetEmail) {
        BlockUserPayload payload;
        std::memset(&payload, 0, sizeof(payload));
        std::strncpy(payload.targetEmail, targetEmail.c_str(), MAX_EMAIL_LEN - 1);
        sendMessageWithPayload(MessageType::BlockUser, payload, "BlockUser");
    }

    void TCPClient::unblockUser(const std::string& targetEmail) {
        UnblockUserPayload payload;
        std::memset(&payload, 0, sizeof(payload));
        std::strncpy(payload.targetEmail, targetEmail.c_str(), MAX_EMAIL_LEN - 1);
        sendMessageWithPayload(MessageType::UnblockUser, payload, "UnblockUser");
    }

    void TCPClient::getFriendsList(uint8_t offset, uint8_t limit) {
        GetFriendsListPayload payload;
        payload.offset = offset;
        payload.limit = limit;
        sendMessageWithPayload(MessageType::GetFriendsList, payload, "GetFriendsList");
    }

    void TCPClient::getFriendRequests() {
        sendMessageNoPayload(MessageType::GetFriendRequests, "GetFriendRequests");
    }

    void TCPClient::getBlockedUsers() {
        sendMessageNoPayload(MessageType::GetBlockedUsers, "GetBlockedUsers");
    }

    // ═══════════════════════════════════════════════════════════════════
    // Private Messaging (Phase 4)
    // ═══════════════════════════════════════════════════════════════════

    void TCPClient::sendPrivateMessage(const std::string& recipientEmail, const std::string& message) {
        SendPrivateMessagePayload payload;
        std::memset(&payload, 0, sizeof(payload));
        std::strncpy(payload.recipientEmail, recipientEmail.c_str(), MAX_EMAIL_LEN - 1);
        std::strncpy(payload.message, message.c_str(), MAX_MESSAGE_LEN - 1);
        sendMessageWithPayload(MessageType::SendPrivateMessage, payload, "SendPrivateMessage");
    }

    void TCPClient::getConversation(const std::string& otherEmail, uint8_t offset, uint8_t limit) {
        GetConversationPayload payload;
        std::memset(&payload, 0, sizeof(payload));
        std::strncpy(payload.otherEmail, otherEmail.c_str(), MAX_EMAIL_LEN - 1);
        payload.offset = offset;
        payload.limit = limit;
        sendMessageWithPayload(MessageType::GetConversation, payload, "GetConversation");
    }

    void TCPClient::getConversationsList() {
        sendMessageNoPayload(MessageType::GetConversationsList, "GetConversationsList");
    }

    void TCPClient::markMessagesRead(const std::string& otherEmail) {
        MarkMessagesReadPayload payload;
        std::memset(&payload, 0, sizeof(payload));
        std::strncpy(payload.otherEmail, otherEmail.c_str(), MAX_EMAIL_LEN - 1);
        sendMessageWithPayload(MessageType::MarkMessagesRead, payload, "MarkMessagesRead");
    }

    // ═══════════════════════════════════════════════════════════════════
    // Room state getters
    // ═══════════════════════════════════════════════════════════════════

    bool TCPClient::isInRoom() const {
        std::scoped_lock lock(_mutex);
        return _currentRoomCode.has_value();
    }

    std::optional<std::string> TCPClient::getCurrentRoomCode() const {
        std::scoped_lock lock(_mutex);
        return _currentRoomCode;
    }

    bool TCPClient::isHost() const {
        std::scoped_lock lock(_mutex);
        return _isHost;
    }

    bool TCPClient::isReady() const {
        std::scoped_lock lock(_mutex);
        return _isReady;
    }

}
