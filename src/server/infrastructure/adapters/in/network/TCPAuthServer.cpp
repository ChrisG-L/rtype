/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** TCPAuthServer - Authentication server on port 4125
*/

#include "infrastructure/adapters/in/network/TCPAuthServer.hpp"
#include "Protocol.hpp"
#include "infrastructure/adapters/in/network/protocol/Command.hpp"
#include "infrastructure/logging/Logger.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <openssl/ssl.h>

namespace infrastructure::adapters::in::network {
    static constexpr int CLIENT_TIMEOUT_MS = 2000;
    static constexpr int TIMEOUT_CHECK_INTERVAL_MS = 1000;

    // Session implementation
    Session::Session(
        ssl::stream<tcp::socket> socket,
        std::shared_ptr<IUserRepository> userRepository,
        std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
        std::shared_ptr<ILeaderboardRepository> leaderboardRepository,
        std::shared_ptr<IIdGenerator> idGenerator,
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<SessionManager> sessionManager,
        std::shared_ptr<RoomManager> roomManager)
    : _socket(std::move(socket)), _isAuthenticated(false),
      _userRepository(userRepository), _userSettingsRepository(userSettingsRepository),
      _leaderboardRepository(leaderboardRepository),
      _idGenerator(idGenerator), _logger(logger),
      _sessionManager(sessionManager), _roomManager(roomManager),
      _timeoutTimer(_socket.get_executor())
    {
        _onAuthSuccess = [this](const User& user) { onLoginSuccess(user); };
        _lastActivity = std::chrono::steady_clock::now();
    }

    Session::~Session() noexcept
    {
        try {
            auto logger = server::logging::Logger::getNetworkLogger();
            if (_isAuthenticated && _user.has_value()) {
                std::string username = _user->getUsername().value();
                std::string email = _user->getEmail().value();
                logger->info("Session closed - removing user '{}' from active sessions", username);

                // Unregister session callbacks
                if (_roomManager) {
                    _roomManager->unregisterSessionCallbacks(email);
                }

                // Notify UDPServer to save stats BEFORE leaving room
                // This ensures stats are persisted on any disconnect (timeout, close, etc.)
                if (_sessionManager) {
                    _sessionManager->notifyPlayerLeaveGame(email);
                    logger->debug("Notified UDPServer to save stats for: {}", email);
                }

                // Remove from room if in one
                if (_roomManager && _roomManager->isPlayerInRoom(email)) {
                    _roomManager->leaveRoom(email);
                    logger->debug("Player removed from room for email: {}", email);
                }

                // Unregister kicked callback from SessionManager
                if (_sessionManager) {
                    _sessionManager->unregisterKickedCallback(email);
                }

                // Remove from SessionManager (cleans up token and allows re-login)
                if (_sessionManager) {
                    _sessionManager->removeSession(email);
                    logger->debug("Session removed from SessionManager for email: {}", email);
                }
            } else {
                logger->debug("Session closed (unauthenticated)");
            }

            // SSL shutdown (graceful close)
            boost::system::error_code ec;
            _socket.shutdown(ec);
            if (ec && ec != boost::asio::error::eof && ec != boost::asio::ssl::error::stream_truncated) {
                logger->debug("SSL shutdown notice: {}", ec.message());
            }
        } catch (...) {
            // Destructors must never throw - silently ignore any exception
        }
    }

    void Session::start()
    {
        do_write(MessageType::Login, "");
        scheduleTimeoutCheck();
        do_read();
    }

    void Session::do_read() {
        auto self = shared_from_this();
        auto logger = server::logging::Logger::getNetworkLogger();

        _socket.async_read_some(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            [this, self, logger](boost::system::error_code ec, std::size_t bytes) {
                if (!ec) {
                    _lastActivity = std::chrono::steady_clock::now();

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

                        handle_command(head);

                        _accumulator.erase(_accumulator.begin(), _accumulator.begin() + totalSize);
                    }

                    do_read();
                }
            }
        );
    }

    void Session::onLoginSuccess(const User& user) {
        _isAuthenticated = true;
        _user = user;
    }

    void Session::do_write(const MessageType& msgType, const std::string& message) {
        auto logger = server::logging::Logger::getNetworkLogger();
        logger->debug("Sending message type: {}, auth: {}", static_cast<uint16_t>(msgType), _isAuthenticated);

        struct Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(msgType),
            .payload_size = static_cast<uint32_t>(message.length())
        };

        const size_t totalSize = Header::WIRE_SIZE + message.length();
        auto buf = std::make_shared<std::vector<uint8_t>>(totalSize);

        head.to_bytes(buf->data());
        memcpy(buf->data() + Header::WIRE_SIZE, message.c_str(), message.length());

        auto self = shared_from_this();
        boost::asio::async_write(
            _socket,
            boost::asio::buffer(buf->data(), totalSize),
            [this, self, buf](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("Write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_auth_response(const MessageType& msgType, const AuthResponse& resp) {
        auto logger = server::logging::Logger::getNetworkLogger();
        logger->debug("Sending auth response: success={}, code={}", resp.success, resp.error_code);

        struct Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(msgType),
            .payload_size = static_cast<uint32_t>(AuthResponse::WIRE_SIZE)
        };

        const size_t totalSize = Header::WIRE_SIZE + AuthResponse::WIRE_SIZE;
        auto buf = std::make_shared<std::vector<uint8_t>>(totalSize);

        head.to_bytes(buf->data());
        resp.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(
            _socket,
            boost::asio::buffer(buf->data(), totalSize),
            [this, self, buf](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("Write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_auth_response_with_token(const MessageType& msgType, const AuthResponseWithToken& resp) {
        auto logger = server::logging::Logger::getNetworkLogger();
        logger->debug("Sending auth response with token: success={}, code={}", resp.success, resp.error_code);

        struct Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(msgType),
            .payload_size = static_cast<uint32_t>(AuthResponseWithToken::WIRE_SIZE)
        };

        const size_t totalSize = Header::WIRE_SIZE + AuthResponseWithToken::WIRE_SIZE;
        auto buf = std::make_shared<std::vector<uint8_t>>(totalSize);

        head.to_bytes(buf->data());
        resp.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(
            _socket,
            boost::asio::buffer(buf->data(), totalSize),
            [this, self, buf](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("Write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_heartbeat_ack() {
        struct Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::HeartBeatAck),
            .payload_size = 0
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE);
        head.to_bytes(buf->data());

        auto self = shared_from_this();
        boost::asio::async_write(
            _socket,
            boost::asio::buffer(buf->data(), Header::WIRE_SIZE),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("HeartBeatAck write error: {}", ec.message());
                }
            });
    }

    void Session::scheduleTimeoutCheck() {
        auto self = shared_from_this();
        _timeoutTimer.expires_after(std::chrono::milliseconds(TIMEOUT_CHECK_INTERVAL_MS));
        _timeoutTimer.async_wait([this, self](boost::system::error_code ec) {
            if (ec) {
                return;
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - _lastActivity
            ).count();

            if (elapsed > CLIENT_TIMEOUT_MS) {
                auto logger = server::logging::Logger::getNetworkLogger();
                logger->warn("TCP Client heartbeat timeout ({}ms) - closing session", elapsed);

                boost::system::error_code closeEc;
                _socket.lowest_layer().close(closeEc);
                return;
            }

            scheduleTimeoutCheck();
        });
    }

    void Session::handle_command(const Header& head) {
        using application::use_cases::auth::Login;
        using application::use_cases::auth::Register;
        auto networkLogger = server::logging::Logger::getNetworkLogger();

        // Handle HeartBeat separately
        if (head.type == static_cast<uint16_t>(MessageType::HeartBeat)) {
            do_write_heartbeat_ack();
            return;
        }

        // Extract payload for room handlers
        std::vector<uint8_t> payload(_accumulator.begin() + Header::WIRE_SIZE,
                                     _accumulator.begin() + Header::WIRE_SIZE + head.payload_size);

        // Handle room messages (requires authentication)
        if (_isAuthenticated && _user.has_value()) {
            switch (static_cast<MessageType>(head.type)) {
                case MessageType::CreateRoom:
                    handleCreateRoom(payload);
                    return;
                case MessageType::JoinRoomByCode:
                    handleJoinRoomByCode(payload);
                    return;
                case MessageType::LeaveRoom:
                    handleLeaveRoom();
                    return;
                case MessageType::SetReady:
                    handleSetReady(payload);
                    return;
                case MessageType::StartGame:
                    handleStartGame();
                    return;
                case MessageType::KickPlayer:
                    handleKickPlayer(payload);
                    return;
                case MessageType::SetRoomConfig:
                    handleSetRoomConfig(payload);
                    return;
                case MessageType::BrowsePublicRooms:
                    handleBrowsePublicRooms();
                    return;
                case MessageType::QuickJoin:
                    handleQuickJoin();
                    return;
                case MessageType::GetUserSettings:
                    handleGetUserSettings();
                    return;
                case MessageType::SaveUserSettings:
                    handleSaveUserSettings(payload);
                    return;
                case MessageType::SendChatMessage:
                    handleSendChatMessage(payload);
                    return;
                // Leaderboard messages
                case MessageType::GetLeaderboard:
                    handleGetLeaderboard(payload);
                    return;
                case MessageType::GetPlayerStats:
                    handleGetPlayerStats();
                    return;
                case MessageType::GetGameHistory:
                    handleGetGameHistory();
                    return;
                case MessageType::GetAchievements:
                    handleGetAchievements();
                    return;
                default:
                    break;
            }
        }

        // Determine response type based on request type
        MessageType responseType = (head.type == static_cast<uint16_t>(MessageType::Login))
            ? MessageType::LoginAck
            : MessageType::RegisterAck;

        try {
            // Inline auth logic (previously in Execute/ExecuteAuth)
            std::shared_ptr<Login> login = std::make_shared<Login>(_userRepository, _logger);
            std::shared_ptr<Register> registerUseCase = std::make_shared<Register>(_userRepository, _idGenerator, _logger);

            std::optional<User> userOpt;
            if (head.type == static_cast<uint16_t>(MessageType::Login)) {
                auto loginOpt = LoginMessage::from_bytes(payload.data(), payload.size());
                if (loginOpt) {
                    userOpt = login->execute(loginOpt->username, loginOpt->password);
                } else {
                    networkLogger->warn("Invalid LoginMessage received!");
                }
            } else if (head.type == static_cast<uint16_t>(MessageType::Register)) {
                auto registerOpt = RegisterMessage::from_bytes(payload.data(), payload.size());
                if (registerOpt) {
                    userOpt = registerUseCase->execute(registerOpt->username, registerOpt->email, registerOpt->password);
                } else {
                    networkLogger->warn("Invalid RegisterMessage received!");
                }
            }

            if (userOpt.has_value()) {
                _onAuthSuccess(userOpt.value());
            }

            // Check if authentication succeeded
            if (_isAuthenticated && _user.has_value()) {
                std::string email = _user->getEmail().value();
                std::string displayName = _user->getUsername().value();

                // Check if user is banned - return same error as invalid password
                if (_sessionManager->isBanned(email)) {
                    _isAuthenticated = false;
                    networkLogger->warn("Banned user {} attempted to login", email);
                    AuthResponse resp;
                    resp.success = false;
                    std::snprintf(resp.error_code, MAX_ERROR_CODE_LEN, "%s", "INVALID_CREDENTIALS");
                    std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", "Invalid username or password");
                    do_write_auth_response(responseType, resp);
                    return;
                }

                // Create a session via SessionManager
                auto sessionResult = _sessionManager->createSession(email, displayName);
                if (sessionResult) {
                    _sessionToken = sessionResult->token;
                    networkLogger->info("Authentication successful, session created for {}", email);

                    // Load GodMode state from database (hidden feature)
                    if (_userSettingsRepository) {
                        auto settingsOpt = _userSettingsRepository->findByEmail(email);
                        if (settingsOpt && settingsOpt->godMode) {
                            _sessionManager->setGodMode(email, true);
                        }
                    }

                    // Register session callbacks for room broadcasts
                    if (_roomManager) {
                        auto weakSelf = weak_from_this();
                        _roomManager->registerSessionCallbacks(
                            email,
                            [weakSelf](const RoomUpdate& update) {
                                if (auto self = weakSelf.lock()) {
                                    self->do_write_room_update(update);
                                }
                            },
                            [weakSelf](const GameStarting& gs) {
                                if (auto self = weakSelf.lock()) {
                                    self->do_write_game_starting(gs);
                                }
                            }
                        );
                        _roomManager->registerKickedCallback(
                            email,
                            [weakSelf](const PlayerKickedNotification& notif) {
                                if (auto self = weakSelf.lock()) {
                                    self->do_write_player_kicked(notif);
                                }
                            }
                        );
                        _roomManager->registerChatCallback(
                            email,
                            [weakSelf](const ChatMessagePayload& msg) {
                                if (auto self = weakSelf.lock()) {
                                    self->do_write_chat_message(msg);
                                }
                            }
                        );
                    }

                    // Register kicked callback in SessionManager for in-game kicks
                    if (_sessionManager) {
                        auto weakSelf = weak_from_this();
                        _sessionManager->registerKickedCallback(
                            email,
                            [weakSelf](const std::string& reason) {
                                if (auto self = weakSelf.lock()) {
                                    PlayerKickedNotification notif;
                                    std::memset(notif.reason, 0, MAX_ERROR_MSG_LEN);
                                    std::snprintf(notif.reason, MAX_ERROR_MSG_LEN, "%s", reason.c_str());
                                    self->do_write_player_kicked(notif);
                                }
                            }
                        );
                    }

                    AuthResponseWithToken resp;
                    resp.success = true;
                    std::snprintf(resp.error_code, MAX_ERROR_CODE_LEN, "%s", "");
                    std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", "Authentication successful");
                    resp.token = sessionResult->token;
                    do_write_auth_response_with_token(responseType, resp);
                } else {
                    // User already has an active session
                    _isAuthenticated = false;
                    networkLogger->warn("User {} already has an active session", email);
                    AuthResponse resp;
                    resp.success = false;
                    std::snprintf(resp.error_code, MAX_ERROR_CODE_LEN, "%s", "ALREADY_CONNECTED");
                    std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", "User already has an active session");
                    do_write_auth_response(responseType, resp);
                }
            } else {
                networkLogger->info("Authentication failed - invalid credentials");
                AuthResponse resp;
                resp.success = false;
                std::snprintf(resp.error_code, MAX_ERROR_CODE_LEN, "%s", "INVALID_CREDENTIALS");
                std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", "Invalid username or password");
                do_write_auth_response(responseType, resp);
            }
        } catch (const domain::exceptions::user::UsernameAlreadyExistsException& e) {
            networkLogger->warn("Username already exists: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::snprintf(resp.error_code, MAX_ERROR_CODE_LEN, "%s", "USERNAME_EXISTS");
            std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", e.what());
            do_write_auth_response(MessageType::RegisterAck, resp);
        } catch (const domain::exceptions::user::EmailAlreadyExistsException& e) {
            networkLogger->warn("Email already exists: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::snprintf(resp.error_code, MAX_ERROR_CODE_LEN, "%s", "EMAIL_EXISTS");
            std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", e.what());
            do_write_auth_response(MessageType::RegisterAck, resp);
        } catch (const domain::exceptions::user::UsernameException& e) {
            networkLogger->warn("Username validation error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::snprintf(resp.error_code, MAX_ERROR_CODE_LEN, "%s", "INVALID_USERNAME");
            std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", e.what());
            do_write_auth_response(responseType, resp);
        } catch (const domain::exceptions::user::EmailException& e) {
            networkLogger->warn("Email validation error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::snprintf(resp.error_code, MAX_ERROR_CODE_LEN, "%s", "INVALID_EMAIL");
            std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", e.what());
            do_write_auth_response(MessageType::RegisterAck, resp);
        } catch (const domain::exceptions::user::PasswordException& e) {
            networkLogger->warn("Password validation error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::snprintf(resp.error_code, MAX_ERROR_CODE_LEN, "%s", "INVALID_PASSWORD");
            std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", e.what());
            do_write_auth_response(responseType, resp);
        } catch (const domain::exceptions::DomainException& e) {
            networkLogger->error("Domain error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::snprintf(resp.error_code, MAX_ERROR_CODE_LEN, "%s", "DOMAIN_ERROR");
            std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", e.what());
            do_write_auth_response(responseType, resp);
        } catch (const std::exception& e) {
            networkLogger->error("Server error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::snprintf(resp.error_code, MAX_ERROR_CODE_LEN, "%s", "SERVER_ERROR");
            std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", e.what());
            do_write_auth_response(responseType, resp);
        }
    }

    // TCPAuthServer implementation
    TCPAuthServer::TCPAuthServer(
        boost::asio::io_context& io_ctx,
        const std::string& certFile,
        const std::string& keyFile,
        std::shared_ptr<IUserRepository> userRepository,
        std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
        std::shared_ptr<ILeaderboardRepository> leaderboardRepository,
        std::shared_ptr<IIdGenerator> idGenerator,
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<SessionManager> sessionManager,
        std::shared_ptr<RoomManager> roomManager)
        : _io_ctx(io_ctx)
        , _sslContext(ssl::context::tlsv12_server)
        , _certFile(certFile)
        , _keyFile(keyFile)
        , _userRepository(userRepository)
        , _userSettingsRepository(userSettingsRepository)
        , _leaderboardRepository(leaderboardRepository)
        , _idGenerator(idGenerator)
        , _logger(logger)
        , _sessionManager(sessionManager)
        , _roomManager(roomManager)
        , _acceptor(io_ctx, tcp::endpoint(tcp::v4(), 4125))
    {
        initSSLContext();

        auto networkLogger = server::logging::Logger::getNetworkLogger();
        networkLogger->info("TCP Auth Server started on port 4125 with TLS 1.2+");
    }

    void TCPAuthServer::initSSLContext() {
        auto networkLogger = server::logging::Logger::getNetworkLogger();

        // Disable obsolete protocols (SSLv2, SSLv3, TLS 1.0, TLS 1.1)
        _sslContext.set_options(
            ssl::context::default_workarounds |
            ssl::context::no_sslv2 |
            ssl::context::no_sslv3 |
            ssl::context::no_tlsv1 |
            ssl::context::no_tlsv1_1 |
            ssl::context::single_dh_use
        );

        // Force TLS 1.2 minimum
        SSL_CTX_set_min_proto_version(_sslContext.native_handle(), TLS1_2_VERSION);

        // Load certificate and private key
        try {
            _sslContext.use_certificate_chain_file(_certFile);
            networkLogger->info("TLS Certificate loaded: {}", _certFile);

            _sslContext.use_private_key_file(_keyFile, ssl::context::pem);
            networkLogger->info("TLS Private key loaded: {}", _keyFile);
        } catch (const std::exception& e) {
            networkLogger->error("Failed to load TLS certificates: {}", e.what());
            throw;
        }

        // Modern cipher suites (AEAD only, forward secrecy)
        SSL_CTX_set_cipher_list(_sslContext.native_handle(),
            "ECDHE-ECDSA-AES256-GCM-SHA384:"
            "ECDHE-RSA-AES256-GCM-SHA384:"
            "ECDHE-ECDSA-AES128-GCM-SHA256:"
            "ECDHE-RSA-AES128-GCM-SHA256:"
            "ECDHE-ECDSA-CHACHA20-POLY1305:"
            "ECDHE-RSA-CHACHA20-POLY1305"
        );

        networkLogger->info("TLS context initialized successfully");
    }

    void TCPAuthServer::start() {
        start_accept();
    }

    void TCPAuthServer::run() {
        _io_ctx.run();
    }

    void TCPAuthServer::stop() {
        _acceptor.close();
    }

    void TCPAuthServer::start_accept() {
        auto networkLogger = server::logging::Logger::getNetworkLogger();

        // Create a new ssl::stream for each incoming connection
        auto sslSocket = std::make_shared<ssl::stream<tcp::socket>>(_io_ctx, _sslContext);

        _acceptor.async_accept(
            sslSocket->lowest_layer(),  // Accept on the underlying TCP socket
            [this, sslSocket, networkLogger](boost::system::error_code ec) {
                if (ec) {
                    if (ec != boost::asio::error::operation_aborted) {
                        networkLogger->error("Accept error: {}", ec.message());
                    }
                    // Continue accepting even on error
                    start_accept();
                    return;
                }

                // Get endpoint safely - client may disconnect between accept and this call
                boost::system::error_code epEc;
                auto clientEndpoint = sslSocket->lowest_layer().remote_endpoint(epEc);
                if (epEc) {
                    networkLogger->warn("Client disconnected before handshake: {}", epEc.message());
                    start_accept();
                    return;
                }

                networkLogger->debug("TCP connection from {}, starting TLS handshake...",
                    clientEndpoint.address().to_string());

                // Async TLS handshake
                sslSocket->async_handshake(
                    ssl::stream_base::server,
                    [this, sslSocket, networkLogger, clientEndpoint](boost::system::error_code hsError) {
                        if (hsError) {
                            networkLogger->warn("TLS handshake failed from {}: {}",
                                clientEndpoint.address().to_string(), hsError.message());
                            // Don't create session if handshake fails
                            return;
                        }

                        networkLogger->info("TLS handshake successful from {}",
                            clientEndpoint.address().to_string());

                        // Create the session with the secured SSL socket
                        auto session = std::make_shared<Session>(
                            std::move(*sslSocket),
                            _userRepository,
                            _userSettingsRepository,
                            _leaderboardRepository,
                            _idGenerator,
                            _logger,
                            _sessionManager,
                            _roomManager
                        );
                        session->start();
                    }
                );

                // Continue accepting new connections
                start_accept();
            }
        );
    }

    // =========================================================================
    // Room Handlers Implementation
    // =========================================================================

    void Session::handleCreateRoom(const std::vector<uint8_t>& payload) {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();
        std::string displayName = _user->getUsername().value();

        auto reqOpt = CreateRoomRequest::from_bytes(payload.data(), payload.size());
        if (!reqOpt) {
            logger->warn("Invalid CreateRoom payload from {}", email);
            CreateRoomAck ack;
            ack.success = 0;
            std::memset(ack.roomCode, 0, ROOM_CODE_LEN);
            std::snprintf(ack.errorCode, MAX_ERROR_CODE_LEN, "%s", "INVALID_REQUEST");
            std::snprintf(ack.message, MAX_ERROR_MSG_LEN, "%s", "Invalid request format");
            do_write_create_room_ack(ack);
            return;
        }

        auto& req = *reqOpt;

        // Validate max players
        if (req.maxPlayers < MIN_ROOM_PLAYERS || req.maxPlayers > MAX_ROOM_PLAYERS) {
            CreateRoomAck ack;
            ack.success = 0;
            std::memset(ack.roomCode, 0, ROOM_CODE_LEN);
            std::snprintf(ack.errorCode, MAX_ERROR_CODE_LEN, "%s", "INVALID_PLAYERS");
            std::snprintf(ack.message, MAX_ERROR_MSG_LEN, "%s", "Max players must be between 2 and 6");
            do_write_create_room_ack(ack);
            return;
        }

        // Get player's ship skin from their settings
        uint8_t shipSkin = 1;  // Default
        if (_userSettingsRepository) {
            auto settingsOpt = _userSettingsRepository->findByEmail(email);
            if (settingsOpt) {
                shipSkin = settingsOpt->shipSkin;
            }
        }

        auto result = _roomManager->createRoom(email, displayName, req.name,
                                                req.maxPlayers, req.isPrivate != 0, shipSkin);
        if (!result) {
            logger->warn("Failed to create room for {} (already in room?)", email);
            CreateRoomAck ack;
            ack.success = 0;
            std::memset(ack.roomCode, 0, ROOM_CODE_LEN);
            std::snprintf(ack.errorCode, MAX_ERROR_CODE_LEN, "%s", "ALREADY_IN_ROOM");
            std::snprintf(ack.message, MAX_ERROR_MSG_LEN, "%s", "You are already in a room");
            do_write_create_room_ack(ack);
            return;
        }

        logger->info("Room '{}' created by {} with code {}", req.name, email, result->code);

        CreateRoomAck ack;
        ack.success = 1;
        std::memcpy(ack.roomCode, result->code.c_str(), ROOM_CODE_LEN);
        std::snprintf(ack.errorCode, MAX_ERROR_CODE_LEN, "%s", "");
        std::snprintf(ack.message, MAX_ERROR_MSG_LEN, "%s", "Room created successfully");
        do_write_create_room_ack(ack);

        // Broadcast room update to all members (just the host at this point)
        broadcastRoomUpdate(result->room);
    }

    void Session::handleJoinRoomByCode(const std::vector<uint8_t>& payload) {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();
        std::string displayName = _user->getUsername().value();

        auto reqOpt = JoinRoomByCodeRequest::from_bytes(payload.data(), payload.size());
        if (!reqOpt) {
            logger->warn("Invalid JoinRoomByCode payload from {}", email);
            JoinRoomNack nack;
            std::snprintf(nack.errorCode, MAX_ERROR_CODE_LEN, "%s", "INVALID_REQUEST");
            std::snprintf(nack.message, MAX_ERROR_MSG_LEN, "%s", "Invalid request format");
            do_write_join_room_nack(nack);
            return;
        }

        std::string code(reqOpt->roomCode, ROOM_CODE_LEN);

        // Get player's ship skin from their settings
        uint8_t shipSkin = 1;  // Default
        if (_userSettingsRepository) {
            auto settingsOpt = _userSettingsRepository->findByEmail(email);
            if (settingsOpt) {
                shipSkin = settingsOpt->shipSkin;
            }
        }

        auto result = _roomManager->joinRoomByCode(code, email, displayName, shipSkin);
        if (!result) {
            logger->warn("Failed to join room {} for {}", code, email);
            JoinRoomNack nack;
            std::snprintf(nack.errorCode, MAX_ERROR_CODE_LEN, "%s", "JOIN_FAILED");
            std::snprintf(nack.message, MAX_ERROR_MSG_LEN, "%s", "Room not found, full, or you're already in a room");
            do_write_join_room_nack(nack);
            return;
        }

        logger->info("{} joined room {} (slot {})", email, code, result->slotId);

        // Use factorized join success handling
        sendJoinSuccessResponse(*result, MessageType::JoinRoomAck);
    }

    void Session::handleLeaveRoom() {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();
        logger->info("handleLeaveRoom called for {}", email);

        // Notify UDPServer to clean up GameWorld before leaving room
        // This will clear the UDP binding and remove player from game instance
        if (_sessionManager) {
            logger->info("Calling notifyPlayerLeaveGame for {}", email);
            _sessionManager->notifyPlayerLeaveGame(email);
        } else {
            logger->warn("SessionManager is null in handleLeaveRoom!");
        }

        // Get room before leaving (for broadcast)
        auto* room = _roomManager->getRoomByPlayerEmail(email);
        std::string code = _roomManager->leaveRoom(email);

        if (code.empty()) {
            logger->debug("{} tried to leave but was not in a room", email);
        } else {
            logger->info("{} left room {}", email, code);

            // Broadcast to remaining members
            auto* remainingRoom = _roomManager->getRoomByCode(code);
            if (remainingRoom && !remainingRoom->isEmpty()) {
                broadcastRoomUpdate(remainingRoom);
            }
        }

        do_write_leave_room_ack();
    }

    void Session::handleSetReady(const std::vector<uint8_t>& payload) {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();

        auto reqOpt = SetReadyRequest::from_bytes(payload.data(), payload.size());
        if (!reqOpt) {
            return;
        }

        bool ready = reqOpt->isReady != 0;
        auto* room = _roomManager->setReady(email, ready);

        if (!room) {
            logger->warn("{} tried to set ready but not in a room", email);
            return;
        }

        logger->debug("{} set ready={}", email, ready);

        SetReadyAck ack;
        ack.isReady = ready ? 1 : 0;
        do_write_set_ready_ack(ack);

        // Broadcast room update
        broadcastRoomUpdate(room);
    }

    void Session::handleStartGame() {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();

        if (!_roomManager->tryStartGame(email)) {
            logger->warn("{} failed to start game (not host or conditions not met)", email);
            StartGameNack nack;
            std::snprintf(nack.errorCode, MAX_ERROR_CODE_LEN, "%s", "CANNOT_START");
            std::snprintf(nack.message, MAX_ERROR_MSG_LEN, "%s", "Cannot start: not host or not enough ready players");
            do_write_start_game_nack(nack);
            return;
        }

        auto* room = _roomManager->getRoomByPlayerEmail(email);
        if (!room) {
            return;
        }

        logger->info("Game starting in room {} by host {}", room->getCode(), email);

        // Set room game speed and room code on all member sessions
        // (for UDPServer to read on JoinGame and route to correct GameWorld)
        std::string roomCode = room->getCode();
        uint16_t gameSpeedPercent = room->getGameSpeedPercent();
        auto memberEmails = _roomManager->getRoomMemberEmails(roomCode);
        for (const auto& memberEmail : memberEmails) {
            _sessionManager->setRoomGameSpeed(memberEmail, gameSpeedPercent);
            _sessionManager->setRoomCode(memberEmail, roomCode);
        }
        logger->debug("Set game speed {}% and room code '{}' for {} members",
                      gameSpeedPercent, roomCode, memberEmails.size());

        do_write_start_game_ack();

        // Broadcast countdown (3, 2, 1, 0)
        // For now, just send countdown=0 to start immediately
        // TODO: Implement actual countdown with timer
        broadcastGameStarting(room, 0);

        // Transition room to InGame state
        room->startGame();
    }

    // =========================================================================
    // Room Response Writers
    // =========================================================================

    void Session::do_write_create_room_ack(const CreateRoomAck& ack) {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::CreateRoomAck),
            .payload_size = static_cast<uint32_t>(CreateRoomAck::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + CreateRoomAck::WIRE_SIZE);
        head.to_bytes(buf->data());
        ack.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("CreateRoomAck write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_join_room_ack(const JoinRoomAck& ack) {
        size_t payloadSize = ack.wire_size();
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::JoinRoomAck),
            .payload_size = static_cast<uint32_t>(payloadSize)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + payloadSize);
        head.to_bytes(buf->data());
        ack.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("JoinRoomAck write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_join_room_nack(const JoinRoomNack& nack) {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::JoinRoomNack),
            .payload_size = static_cast<uint32_t>(JoinRoomNack::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + JoinRoomNack::WIRE_SIZE);
        head.to_bytes(buf->data());
        nack.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("JoinRoomNack write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_leave_room_ack() {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::LeaveRoomAck),
            .payload_size = 0
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE);
        head.to_bytes(buf->data());

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("LeaveRoomAck write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_set_ready_ack(const SetReadyAck& ack) {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::SetReadyAck),
            .payload_size = static_cast<uint32_t>(SetReadyAck::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + SetReadyAck::WIRE_SIZE);
        head.to_bytes(buf->data());
        ack.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("SetReadyAck write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_start_game_ack() {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::StartGameAck),
            .payload_size = 0
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE);
        head.to_bytes(buf->data());

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("StartGameAck write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_start_game_nack(const StartGameNack& nack) {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::StartGameNack),
            .payload_size = static_cast<uint32_t>(StartGameNack::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + StartGameNack::WIRE_SIZE);
        head.to_bytes(buf->data());
        nack.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("StartGameNack write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_room_update(const RoomUpdate& update) {
        size_t payloadSize = update.wire_size();
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::RoomUpdate),
            .payload_size = static_cast<uint32_t>(payloadSize)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + payloadSize);
        head.to_bytes(buf->data());
        update.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("RoomUpdate write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_game_starting(const GameStarting& gs) {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::GameStarting),
            .payload_size = static_cast<uint32_t>(GameStarting::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + GameStarting::WIRE_SIZE);
        head.to_bytes(buf->data());
        gs.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("GameStarting write error: {}", ec.message());
                }
            });
    }

    // =========================================================================
    // Broadcast Helpers
    // =========================================================================

    void Session::broadcastRoomUpdate(domain::entities::Room* room) {
        if (!room || !_roomManager) return;

        // Use RoomManager's broadcast to send to all room members
        _roomManager->broadcastRoomUpdate(room);
    }

    void Session::broadcastGameStarting(domain::entities::Room* room, uint8_t countdown) {
        if (!room || !_roomManager) return;

        // Use RoomManager's broadcast to send to all room members
        _roomManager->broadcastGameStarting(room, countdown);
    }

    // =========================================================================
    // Kick System Implementation (Phase 2)
    // =========================================================================

    void Session::handleKickPlayer(const std::vector<uint8_t>& payload) {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();

        auto reqOpt = KickPlayerRequest::from_bytes(payload.data(), payload.size());
        if (!reqOpt) {
            logger->warn("Invalid KickPlayer payload from {}", email);
            return;
        }

        std::string targetEmail(reqOpt->email);
        std::string reason(reqOpt->reason);

        // Try to kick the player
        auto result = _roomManager->kickPlayer(email, targetEmail, reason);
        if (!result) {
            logger->warn("{} failed to kick {} (not host or invalid target)", email, targetEmail);
            return;
        }

        logger->info("{} kicked {} from room {} (reason: {})",
                     email, targetEmail, result->roomCode,
                     reason.empty() ? "none" : reason);

        // Send ack to the host
        do_write_kick_player_ack();

        // Broadcast room update to remaining members
        auto* room = _roomManager->getRoomByCode(result->roomCode);
        if (room && !room->isEmpty()) {
            broadcastRoomUpdate(room);
        }
    }

    void Session::do_write_kick_player_ack() {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::KickPlayerAck),
            .payload_size = 0
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE);
        head.to_bytes(buf->data());

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("KickPlayerAck write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_player_kicked(const PlayerKickedNotification& notif) {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::PlayerKicked),
            .payload_size = static_cast<uint32_t>(PlayerKickedNotification::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + PlayerKickedNotification::WIRE_SIZE);
        head.to_bytes(buf->data());
        notif.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("PlayerKicked write error: {}", ec.message());
                }
            });
    }

    // =========================================================================
    // Room Configuration Implementation
    // =========================================================================

    void Session::handleSetRoomConfig(const std::vector<uint8_t>& payload) {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();

        auto reqOpt = SetRoomConfigRequest::from_bytes(payload.data(), payload.size());
        if (!reqOpt) {
            logger->warn("Invalid SetRoomConfig payload from {}", email);
            do_write_set_room_config_ack(false);
            return;
        }

        // Try to set the game speed
        auto* room = _roomManager->setRoomGameSpeed(email, reqOpt->gameSpeedPercent);
        if (!room) {
            logger->warn("{} failed to set room config (not host or not in room)", email);
            do_write_set_room_config_ack(false);
            return;
        }

        logger->info("{} set game speed to {}%", email, reqOpt->gameSpeedPercent);

        // Send ack to the host
        do_write_set_room_config_ack(true);

        // Broadcast room update to all members
        broadcastRoomUpdate(room);
    }

    void Session::do_write_set_room_config_ack(bool success) {
        SetRoomConfigAck ack;
        ack.success = success ? 1 : 0;

        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::SetRoomConfigAck),
            .payload_size = static_cast<uint32_t>(SetRoomConfigAck::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + SetRoomConfigAck::WIRE_SIZE);
        head.to_bytes(buf->data());
        ack.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("SetRoomConfigAck write error: {}", ec.message());
                }
            });
    }

    // =========================================================================
    // Room Browser Implementation (Phase 2)
    // =========================================================================

    void Session::handleBrowsePublicRooms() {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();

        auto publicRooms = _roomManager->getPublicRooms();

        BrowsePublicRoomsResponse resp;
        resp.roomCount = static_cast<uint8_t>(std::min(publicRooms.size(),
                                                        static_cast<size_t>(MAX_BROWSER_ROOMS)));

        for (uint8_t i = 0; i < resp.roomCount; ++i) {
            const auto& entry = publicRooms[i];
            std::memcpy(resp.rooms[i].code, entry.code.c_str(), ROOM_CODE_LEN);
            std::snprintf(resp.rooms[i].name, ROOM_NAME_LEN, "%s", entry.name.c_str());
            resp.rooms[i].currentPlayers = entry.currentPlayers;
            resp.rooms[i].maxPlayers = entry.maxPlayers;
        }

        logger->debug("BrowsePublicRooms: {} rooms found for {}", resp.roomCount, email);
        do_write_browse_public_rooms(resp);
    }

    void Session::handleQuickJoin() {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();
        std::string displayName = _user->getUsername().value();

        // Get player's ship skin from their settings
        uint8_t shipSkin = 1;  // Default
        if (_userSettingsRepository) {
            auto settingsOpt = _userSettingsRepository->findByEmail(email);
            if (settingsOpt) {
                shipSkin = settingsOpt->shipSkin;
            }
        }

        auto result = _roomManager->quickJoin(email, displayName, shipSkin);
        if (!result) {
            logger->info("QuickJoin failed for {} - no rooms available", email);
            QuickJoinNack nack;
            std::snprintf(nack.errorCode, MAX_ERROR_CODE_LEN, "%s", "NO_ROOMS");
            std::snprintf(nack.message, MAX_ERROR_MSG_LEN, "%s", "No public rooms available");
            do_write_quick_join_nack(nack);
            return;
        }

        logger->info("{} quick-joined room {} (slot {})", email, result->room->getCode(), result->slotId);

        // Use factorized join success handling
        sendJoinSuccessResponse(*result, MessageType::QuickJoinAck);
    }

    void Session::sendJoinSuccessResponse(const RoomManager::JoinResult& result, MessageType ackType) {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();

        // Build JoinRoomAck with player list
        JoinRoomAck ack{};
        ack.slotId = result.slotId;
        std::snprintf(ack.roomName, ROOM_NAME_LEN, "%s", result.room->getName().c_str());
        std::memcpy(ack.roomCode, result.room->getCode().c_str(), ROOM_CODE_LEN);
        ack.maxPlayers = result.room->getMaxPlayers();
        ack.isHost = result.room->isHost(email) ? 1 : 0;

        // Include current player list in the ack (fixes race condition with RoomUpdate)
        const auto& slots = result.room->getSlots();
        ack.playerCount = 0;
        for (size_t i = 0; i < domain::entities::Room::MAX_SLOTS; ++i) {
            if (slots[i].occupied && ack.playerCount < MAX_ROOM_PLAYERS) {
                RoomPlayerState& state = ack.players[ack.playerCount];
                state.slotId = static_cast<uint8_t>(i);
                state.occupied = 1;
                std::snprintf(state.displayName, MAX_USERNAME_LEN, "%s", slots[i].displayName.c_str());
                std::snprintf(state.email, MAX_EMAIL_LEN, "%s", slots[i].email.c_str());
                state.isReady = slots[i].isReady ? 1 : 0;
                state.isHost = slots[i].isHost ? 1 : 0;
                state.shipSkin = slots[i].shipSkin;
                ++ack.playerCount;
            }
        }

        // Send ack with appropriate message type
        size_t payloadSize = ack.wire_size();
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(ackType),
            .payload_size = static_cast<uint32_t>(payloadSize)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + payloadSize);
        head.to_bytes(buf->data());
        ack.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf, ackType](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("{} write error: {}",
                        ackType == MessageType::QuickJoinAck ? "QuickJoinAck" : "JoinRoomAck",
                        ec.message());
                }
            });

        // Send chat history to the joining player
        std::string roomCode = result.room->getCode();
        auto chatHistory = _roomManager->getChatHistory(roomCode);
        if (!chatHistory.empty()) {
            ChatHistoryResponse histResp{};
            histResp.messageCount = static_cast<uint8_t>(std::min(chatHistory.size(), static_cast<size_t>(MAX_CHAT_HISTORY)));
            for (uint8_t i = 0; i < histResp.messageCount; ++i) {
                const auto& msg = chatHistory[i];
                std::snprintf(histResp.messages[i].displayName, MAX_USERNAME_LEN, "%s", msg.displayName.c_str());
                std::snprintf(histResp.messages[i].message, CHAT_MESSAGE_LEN, "%s", msg.message.c_str());
                histResp.messages[i].timestamp = static_cast<uint32_t>(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        msg.timestamp.time_since_epoch()
                    ).count()
                );
            }
            do_write_chat_history(histResp);
        }

        // Broadcast room update to all members
        broadcastRoomUpdate(result.room);
    }

    void Session::do_write_browse_public_rooms(const BrowsePublicRoomsResponse& resp) {
        size_t payloadSize = resp.wire_size();
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::BrowsePublicRoomsAck),
            .payload_size = static_cast<uint32_t>(payloadSize)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + payloadSize);
        head.to_bytes(buf->data());
        resp.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("BrowsePublicRoomsAck write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_quick_join_nack(const QuickJoinNack& nack) {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::QuickJoinNack),
            .payload_size = static_cast<uint32_t>(QuickJoinNack::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + QuickJoinNack::WIRE_SIZE);
        head.to_bytes(buf->data());
        nack.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("QuickJoinNack write error: {}", ec.message());
                }
            });
    }

    // =========================================================================
    // User Settings Implementation (Phase 2)
    // =========================================================================

    void Session::handleGetUserSettings() {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();

        GetUserSettingsResponse resp;

        if (!_userSettingsRepository) {
            logger->warn("UserSettingsRepository not available");
            resp.found = 0;
            // Set defaults
            std::snprintf(resp.settings.colorBlindMode, COLORBLIND_MODE_LEN, "%s", "none");
            resp.settings.gameSpeedPercent = 100;
            // Default key bindings
            UserSettingsData defaults;
            defaults.setDefaultKeyBindings();
            std::memcpy(resp.settings.keyBindings, defaults.keyBindings.data(), KEY_BINDINGS_COUNT);
            resp.settings.shipSkin = defaults.shipSkin;
            do_write_get_user_settings_response(resp);
            return;
        }

        auto settingsOpt = _userSettingsRepository->findByEmail(email);
        if (settingsOpt) {
            resp.found = 1;
            std::snprintf(resp.settings.colorBlindMode, COLORBLIND_MODE_LEN, "%s", settingsOpt->colorBlindMode.c_str());
            resp.settings.gameSpeedPercent = settingsOpt->gameSpeedPercent;
            std::memcpy(resp.settings.keyBindings, settingsOpt->keyBindings.data(), KEY_BINDINGS_COUNT);
            resp.settings.shipSkin = settingsOpt->shipSkin;
            // Voice settings
            resp.settings.voiceMode = settingsOpt->voiceMode;
            resp.settings.vadThreshold = settingsOpt->vadThreshold;
            resp.settings.micGain = settingsOpt->micGain;
            resp.settings.voiceVolume = settingsOpt->voiceVolume;
            // Audio device selection
            std::snprintf(resp.settings.audioInputDevice, AUDIO_DEVICE_NAME_LEN, "%s", settingsOpt->audioInputDevice.c_str());
            std::snprintf(resp.settings.audioOutputDevice, AUDIO_DEVICE_NAME_LEN, "%s", settingsOpt->audioOutputDevice.c_str());
            // Chat settings
            resp.settings.keepChatOpenAfterSend = settingsOpt->keepChatOpenAfterSend ? 1 : 0;
            logger->debug("GetUserSettings: found settings for {} (input='{}', output='{}')",
                email, settingsOpt->audioInputDevice, settingsOpt->audioOutputDevice);
        } else {
            resp.found = 0;
            // Return defaults
            std::snprintf(resp.settings.colorBlindMode, COLORBLIND_MODE_LEN, "%s", "none");
            resp.settings.gameSpeedPercent = 100;
            UserSettingsData defaults;
            defaults.setDefaultKeyBindings();
            std::memcpy(resp.settings.keyBindings, defaults.keyBindings.data(), KEY_BINDINGS_COUNT);
            resp.settings.shipSkin = defaults.shipSkin;
            // Voice defaults
            resp.settings.voiceMode = 0;  // PTT
            resp.settings.vadThreshold = 2;
            resp.settings.micGain = 100;
            resp.settings.voiceVolume = 100;
            // Audio device defaults (empty = auto)
            resp.settings.audioInputDevice[0] = '\0';
            resp.settings.audioOutputDevice[0] = '\0';
            // Chat defaults
            resp.settings.keepChatOpenAfterSend = 0;  // Close after send by default
            logger->debug("GetUserSettings: no settings found for {}, returning defaults", email);
        }

        do_write_get_user_settings_response(resp);
    }

    void Session::handleSaveUserSettings(const std::vector<uint8_t>& payload) {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();

        auto reqOpt = SaveUserSettingsRequest::from_bytes(payload.data(), payload.size());
        if (!reqOpt) {
            logger->warn("Invalid SaveUserSettings payload from {}", email);
            SaveUserSettingsResponse resp;
            resp.success = 0;
            std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", "Invalid request format");
            do_write_save_user_settings_response(resp);
            return;
        }

        if (!_userSettingsRepository) {
            logger->warn("UserSettingsRepository not available");
            SaveUserSettingsResponse resp;
            resp.success = 0;
            std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", "Settings service unavailable");
            do_write_save_user_settings_response(resp);
            return;
        }

        // Convert payload to UserSettingsData
        UserSettingsData data;
        data.colorBlindMode = std::string(reqOpt->settings.colorBlindMode);
        data.gameSpeedPercent = reqOpt->settings.gameSpeedPercent;
        std::memcpy(data.keyBindings.data(), reqOpt->settings.keyBindings, KEY_BINDINGS_COUNT);
        data.shipSkin = reqOpt->settings.shipSkin;
        // Voice settings
        data.voiceMode = reqOpt->settings.voiceMode;
        data.vadThreshold = reqOpt->settings.vadThreshold;
        data.micGain = reqOpt->settings.micGain;
        data.voiceVolume = reqOpt->settings.voiceVolume;
        // Audio device selection
        data.audioInputDevice = std::string(reqOpt->settings.audioInputDevice);
        data.audioOutputDevice = std::string(reqOpt->settings.audioOutputDevice);
        // Chat settings
        data.keepChatOpenAfterSend = (reqOpt->settings.keepChatOpenAfterSend != 0);

        try {
            _userSettingsRepository->save(email, data);
            logger->info("SaveUserSettings: saved settings for {} (input='{}', output='{}')",
                email, data.audioInputDevice, data.audioOutputDevice);

            // If player is in a room, update their ship skin and broadcast to other players
            if (_roomManager) {
                _roomManager->updatePlayerShipSkin(email, data.shipSkin);
            }

            SaveUserSettingsResponse resp;
            resp.success = 1;
            std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", "Settings saved successfully");
            do_write_save_user_settings_response(resp);
        } catch (const std::exception& e) {
            logger->error("SaveUserSettings error for {}: {}", email, e.what());
            SaveUserSettingsResponse resp;
            resp.success = 0;
            std::snprintf(resp.message, MAX_ERROR_MSG_LEN, "%s", "Failed to save settings");
            do_write_save_user_settings_response(resp);
        }
    }

    void Session::do_write_get_user_settings_response(const GetUserSettingsResponse& resp) {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::GetUserSettingsAck),
            .payload_size = static_cast<uint32_t>(GetUserSettingsResponse::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + GetUserSettingsResponse::WIRE_SIZE);
        head.to_bytes(buf->data());
        resp.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("GetUserSettingsAck write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_save_user_settings_response(const SaveUserSettingsResponse& resp) {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::SaveUserSettingsAck),
            .payload_size = static_cast<uint32_t>(SaveUserSettingsResponse::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + SaveUserSettingsResponse::WIRE_SIZE);
        head.to_bytes(buf->data());
        resp.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("SaveUserSettingsAck write error: {}", ec.message());
                }
            });
    }

    // =========================================================================
    // Chat System Implementation (Phase 2)
    // =========================================================================

    void Session::handleSendChatMessage(const std::vector<uint8_t>& payload) {
        auto logger = server::logging::Logger::getNetworkLogger();
        std::string email = _user->getEmail().value();

        auto reqOpt = SendChatMessageRequest::from_bytes(payload.data(), payload.size());
        if (!reqOpt) {
            logger->warn("Invalid SendChatMessage payload from {}", email);
            return;  // No NACK for chat messages
        }

        std::string message(reqOpt->message);
        if (message.empty()) {
            return;  // Ignore empty messages
        }

        // Hidden command: /toggleGodMode - intercept and don't broadcast
        if (message == "/toggleGodMode") {
            handleToggleGodMode(email);
            do_write_send_chat_message_ack();  // ACK to client so it clears input
            return;  // Don't broadcast to chat
        }

        bool sent = _roomManager->sendChatMessage(email, message);
        if (sent) {
            logger->debug("Chat message from {}: {}", email, message.substr(0, 50));
            do_write_send_chat_message_ack();
        } else {
            logger->debug("Chat message failed (player not in room): {}", email);
        }
    }

    void Session::handleToggleGodMode(const std::string& email) {
        // Toggle GodMode in session
        bool newState = _sessionManager->toggleGodMode(email);

        // Persist to database
        if (_userSettingsRepository) {
            auto settingsOpt = _userSettingsRepository->findByEmail(email);
            if (settingsOpt) {
                settingsOpt->godMode = newState;
                _userSettingsRepository->save(email, *settingsOpt);
            } else {
                // Create default settings with godMode
                application::ports::out::persistence::UserSettingsData newSettings;
                newSettings.setDefaultKeyBindings();
                newSettings.godMode = newState;
                _userSettingsRepository->save(email, newSettings);
            }
        }
    }

    void Session::do_write_send_chat_message_ack() {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::SendChatMessageAck),
            .payload_size = 0
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE);
        head.to_bytes(buf->data());

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("SendChatMessageAck write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_chat_message(const ChatMessagePayload& msg) {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::ChatMessageBroadcast),
            .payload_size = static_cast<uint32_t>(ChatMessagePayload::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + ChatMessagePayload::WIRE_SIZE);
        head.to_bytes(buf->data());
        msg.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("ChatMessageBroadcast write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_chat_history(const ChatHistoryResponse& hist) {
        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::ChatHistory),
            .payload_size = static_cast<uint32_t>(hist.wire_size())
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + hist.wire_size());
        head.to_bytes(buf->data());
        hist.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("ChatHistory write error: {}", ec.message());
                }
            });
    }

    // ========== LEADERBOARD HANDLERS ==========

    void Session::handleGetLeaderboard(const std::vector<uint8_t>& payload) {
        auto logger = server::logging::Logger::getNetworkLogger();

        if (!_isAuthenticated || !_user.has_value()) {
            logger->warn("GetLeaderboard: not authenticated");
            return;
        }

        if (payload.size() < GetLeaderboardRequest::WIRE_SIZE) {
            logger->warn("GetLeaderboard: payload too small");
            return;
        }

        auto reqOpt = GetLeaderboardRequest::from_bytes(payload.data(), payload.size());
        if (!reqOpt) {
            logger->warn("GetLeaderboard: invalid request");
            return;
        }

        auto period = static_cast<application::ports::out::persistence::LeaderboardPeriod>(reqOpt->period);
        uint32_t limit = reqOpt->limit > 0 ? reqOpt->limit : 50;

        auto entries = _leaderboardRepository->getLeaderboard(period, limit);
        uint32_t yourRank = _leaderboardRepository->getPlayerRank(_user->getEmail().value(), period);

        do_write_leaderboard_response(entries, reqOpt->period, yourRank);
        logger->debug("GetLeaderboard: sent {} entries, yourRank={}", entries.size(), yourRank);
    }

    void Session::handleGetPlayerStats() {
        auto logger = server::logging::Logger::getNetworkLogger();

        if (!_isAuthenticated || !_user.has_value()) {
            logger->warn("GetPlayerStats: not authenticated");
            return;
        }

        auto statsOpt = _leaderboardRepository->getPlayerStats(_user->getEmail().value());

        if (statsOpt) {
            do_write_player_stats_response(*statsOpt);
            logger->debug("GetPlayerStats: sent stats for {}", _user->getUsername().value());
        } else {
            // Send empty stats for new player
            application::ports::out::persistence::PlayerStats emptyStats;
            emptyStats.playerName = _user->getUsername().value();
            do_write_player_stats_response(emptyStats);
            logger->debug("GetPlayerStats: sent empty stats for new player {}", _user->getUsername().value());
        }
    }

    void Session::handleGetGameHistory() {
        auto logger = server::logging::Logger::getNetworkLogger();

        if (!_isAuthenticated || !_user.has_value()) {
            logger->warn("GetGameHistory: not authenticated");
            return;
        }

        auto entries = _leaderboardRepository->getGameHistory(_user->getEmail().value(), 10);
        do_write_game_history_response(entries);
        logger->debug("GetGameHistory: sent {} entries", entries.size());
    }

    void Session::handleGetAchievements() {
        auto logger = server::logging::Logger::getNetworkLogger();

        if (!_isAuthenticated || !_user.has_value()) {
            logger->warn("GetAchievements: not authenticated");
            return;
        }

        auto achievements = _leaderboardRepository->getAchievements(_user->getEmail().value());
        do_write_achievements_response(achievements);
        logger->debug("GetAchievements: sent {} achievements", achievements.size());
    }

    // ========== LEADERBOARD RESPONSE WRITERS ==========

    void Session::do_write_leaderboard_response(
        const std::vector<application::ports::out::persistence::LeaderboardEntry>& entries,
        uint8_t period,
        uint32_t yourRank)
    {
        // Build wire format response
        uint8_t entryCount = static_cast<uint8_t>(std::min(entries.size(), size_t(50)));

        // Calculate payload size: header (6 bytes) + entries
        size_t payloadSize = LeaderboardDataResponse::HEADER_SIZE + entryCount * LeaderboardEntryWire::WIRE_SIZE;

        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::LeaderboardData),
            .payload_size = static_cast<uint32_t>(payloadSize)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + payloadSize);
        head.to_bytes(buf->data());

        // Write LeaderboardDataResponse header
        uint8_t* ptr = buf->data() + Header::WIRE_SIZE;
        ptr[0] = period;
        ptr[1] = entryCount;
        uint32_t netRank = swap32(yourRank);
        std::memcpy(ptr + 2, &netRank, 4);
        ptr += LeaderboardDataResponse::HEADER_SIZE;

        // Write each entry
        for (size_t i = 0; i < entryCount; ++i) {
            LeaderboardEntryWire wire;
            wire.rank = entries[i].rank;
            wire.score = entries[i].score;
            wire.kills = static_cast<uint16_t>(entries[i].kills);
            wire.wave = entries[i].wave;
            wire.duration = entries[i].duration;
            wire.timestamp = entries[i].timestamp;
            // Safe string copy: truncate if too long, always null-terminate
            const auto& name = entries[i].playerName;
            const size_t copyLen = std::min(name.size(), static_cast<size_t>(PLAYER_NAME_LEN - 1));
            std::memcpy(wire.playerName, name.c_str(), copyLen);
            std::memset(wire.playerName + copyLen, '\0', PLAYER_NAME_LEN - copyLen);
            wire.to_bytes(ptr);
            ptr += LeaderboardEntryWire::WIRE_SIZE;
        }

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("LeaderboardData write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_player_stats_response(const application::ports::out::persistence::PlayerStats& stats) {
        PlayerStatsWire wire;
        // Safe string copy: truncate if too long, always null-terminate
        const size_t copyLen = std::min(stats.playerName.size(), static_cast<size_t>(PLAYER_NAME_LEN - 1));
        std::memcpy(wire.playerName, stats.playerName.c_str(), copyLen);
        std::memset(wire.playerName + copyLen, '\0', PLAYER_NAME_LEN - copyLen);
        wire.totalScore = stats.totalScore;
        wire.totalKills = stats.totalKills;
        wire.totalDeaths = stats.totalDeaths;
        wire.totalPlaytime = stats.totalPlaytime;
        wire.gamesPlayed = stats.gamesPlayed;
        wire.bestScore = stats.bestScore;
        wire.bestWave = stats.bestWave;
        wire.bestCombo = stats.bestCombo;
        wire.bestKillStreak = stats.bestKillStreak;
        wire.bossKills = stats.bossKills;
        wire.standardKills = stats.standardKills;
        wire.spreadKills = stats.spreadKills;
        wire.laserKills = stats.laserKills;
        wire.missileKills = stats.missileKills;
        wire.achievements = stats.achievements;

        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::PlayerStatsData),
            .payload_size = static_cast<uint32_t>(PlayerStatsWire::WIRE_SIZE)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + PlayerStatsWire::WIRE_SIZE);
        head.to_bytes(buf->data());
        wire.to_bytes(buf->data() + Header::WIRE_SIZE);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("PlayerStatsData write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_game_history_response(
        const std::vector<application::ports::out::persistence::GameHistoryEntry>& entries)
    {
        // Format: count (1 byte) + entries
        uint8_t count = static_cast<uint8_t>(std::min(entries.size(), size_t(10)));
        size_t payloadSize = 1 + count * GameHistoryEntryWire::WIRE_SIZE;

        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::GameHistoryData),
            .payload_size = static_cast<uint32_t>(payloadSize)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + payloadSize);
        head.to_bytes(buf->data());

        uint8_t* ptr = buf->data() + Header::WIRE_SIZE;
        *ptr++ = count;

        for (size_t i = 0; i < count; ++i) {
            GameHistoryEntryWire wire;
            wire.score = entries[i].score;
            wire.wave = entries[i].wave;
            wire.kills = entries[i].kills;
            wire.deaths = entries[i].deaths;
            wire.duration = entries[i].duration;
            wire.timestamp = entries[i].timestamp;
            wire.weaponUsed = entries[i].weaponUsed;
            wire.bossDefeated = entries[i].bossDefeated ? 1 : 0;
            wire.to_bytes(ptr);
            ptr += GameHistoryEntryWire::WIRE_SIZE;
        }

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("GameHistoryData write error: {}", ec.message());
                }
            });
    }

    void Session::do_write_achievements_response(
        const std::vector<application::ports::out::persistence::AchievementRecord>& achievements)
    {
        // Build bitfield from unlocked achievements (unlockedAt > 0 means unlocked)
        uint32_t bitfield = 0;
        for (const auto& ach : achievements) {
            if (ach.unlockedAt > 0) {
                bitfield |= (1u << static_cast<uint8_t>(ach.type));
            }
        }

        // Simple format: just send the bitfield for now
        size_t payloadSize = 4; // just bitfield

        Header head = {
            .isAuthenticated = _isAuthenticated,
            .type = static_cast<uint16_t>(MessageType::AchievementsData),
            .payload_size = static_cast<uint32_t>(payloadSize)
        };

        auto buf = std::make_shared<std::vector<uint8_t>>(Header::WIRE_SIZE + payloadSize);
        head.to_bytes(buf->data());

        uint32_t netBitfield = swap32(bitfield);
        std::memcpy(buf->data() + Header::WIRE_SIZE, &netBitfield, 4);

        auto self = shared_from_this();
        boost::asio::async_write(_socket, boost::asio::buffer(*buf),
            [self, buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    auto logger = server::logging::Logger::getNetworkLogger();
                    logger->error("AchievementsData write error: {}", ec.message());
                }
            });
    }

}
