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
#include <cstring>
#include <vector>

namespace infrastructure::adapters::in::network {
    static constexpr int CLIENT_TIMEOUT_MS = 2000;
    static constexpr int TIMEOUT_CHECK_INTERVAL_MS = 1000;

    // Session implementation
    Session::Session(
        tcp::socket socket,
        std::shared_ptr<IUserRepository> userRepository,
        std::shared_ptr<IIdGenerator> idGenerator,
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<SessionManager> sessionManager)
    : _socket(std::move(socket)), _isAuthenticated(false),
      _userRepository(userRepository), _idGenerator(idGenerator), _logger(logger),
      _sessionManager(sessionManager),
      _timeoutTimer(_socket.get_executor())
    {
        _onAuthSuccess = [this](const User& user) { onLoginSuccess(user); };
        _lastActivity = std::chrono::steady_clock::now();
    }

    Session::~Session()
    {
        auto logger = server::logging::Logger::getNetworkLogger();
        if (_isAuthenticated && _user.has_value()) {
            std::string username = _user->getUsername().value();
            std::string email = _user->getEmail().value();
            logger->info("Session closed - removing user '{}' from active sessions", username);

            // Remove from SessionManager (cleans up token and allows re-login)
            if (_sessionManager) {
                _sessionManager->removeSession(email);
                logger->debug("Session removed from SessionManager for email: {}", email);
            }
        } else {
            logger->debug("Session closed (unauthenticated)");
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

                    logger->debug("Received: {} bytes", bytes);
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
                _socket.close(closeEc);
                return;
            }

            scheduleTimeoutCheck();
        });
    }

    void Session::handle_command(const Header& head) {
        using infrastructure::adapters::in::network::execute::Execute;
        auto networkLogger = server::logging::Logger::getNetworkLogger();

        // Handle HeartBeat separately - no Execute needed
        if (head.type == static_cast<uint16_t>(MessageType::HeartBeat)) {
            do_write_heartbeat_ack();
            return;
        }

        Command cmd = {.type = head.type, .buf = std::vector<uint8_t>(_accumulator.begin() + Header::WIRE_SIZE, _accumulator.begin() + Header::WIRE_SIZE + head.payload_size)};

        // Determine response type based on request type
        MessageType responseType = (head.type == static_cast<uint16_t>(MessageType::Login))
            ? MessageType::LoginAck
            : MessageType::RegisterAck;

        try {
            Execute execute(cmd, _userRepository, _idGenerator, _logger, _onAuthSuccess);

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
                    std::strncpy(resp.error_code, "INVALID_CREDENTIALS", MAX_ERROR_CODE_LEN);
                    std::strncpy(resp.message, "Invalid username or password", MAX_ERROR_MSG_LEN);
                    do_write_auth_response(responseType, resp);
                    return;
                }

                // Create a session via SessionManager
                auto sessionResult = _sessionManager->createSession(email, displayName);
                if (sessionResult) {
                    _sessionToken = sessionResult->token;
                    networkLogger->info("Authentication successful, session created for {}", email);

                    AuthResponseWithToken resp;
                    resp.success = true;
                    std::strncpy(resp.error_code, "", MAX_ERROR_CODE_LEN);
                    std::strncpy(resp.message, "Authentication successful", MAX_ERROR_MSG_LEN);
                    resp.token = sessionResult->token;
                    do_write_auth_response_with_token(responseType, resp);
                } else {
                    // User already has an active session
                    _isAuthenticated = false;
                    networkLogger->warn("User {} already has an active session", email);
                    AuthResponse resp;
                    resp.success = false;
                    std::strncpy(resp.error_code, "ALREADY_CONNECTED", MAX_ERROR_CODE_LEN);
                    std::strncpy(resp.message, "User already has an active session", MAX_ERROR_MSG_LEN);
                    do_write_auth_response(responseType, resp);
                }
            } else {
                networkLogger->info("Authentication failed - invalid credentials");
                AuthResponse resp;
                resp.success = false;
                std::strncpy(resp.error_code, "INVALID_CREDENTIALS", MAX_ERROR_CODE_LEN);
                std::strncpy(resp.message, "Invalid username or password", MAX_ERROR_MSG_LEN);
                do_write_auth_response(responseType, resp);
            }
        } catch (const domain::exceptions::user::UsernameAlreadyExistsException& e) {
            networkLogger->warn("Username already exists: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "USERNAME_EXISTS", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(MessageType::RegisterAck, resp);
        } catch (const domain::exceptions::user::EmailAlreadyExistsException& e) {
            networkLogger->warn("Email already exists: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "EMAIL_EXISTS", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(MessageType::RegisterAck, resp);
        } catch (const domain::exceptions::user::UsernameException& e) {
            networkLogger->warn("Username validation error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "INVALID_USERNAME", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(responseType, resp);
        } catch (const domain::exceptions::user::EmailException& e) {
            networkLogger->warn("Email validation error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "INVALID_EMAIL", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(MessageType::RegisterAck, resp);
        } catch (const domain::exceptions::user::PasswordException& e) {
            networkLogger->warn("Password validation error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "INVALID_PASSWORD", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(responseType, resp);
        } catch (const domain::exceptions::DomainException& e) {
            networkLogger->error("Domain error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "DOMAIN_ERROR", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(responseType, resp);
        } catch (const std::exception& e) {
            networkLogger->error("Server error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "SERVER_ERROR", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(responseType, resp);
        }
    }

    // TCPAuthServer implementation
    TCPAuthServer::TCPAuthServer(
        boost::asio::io_context& io_ctx,
        std::shared_ptr<IUserRepository> userRepository,
        std::shared_ptr<IIdGenerator> idGenerator,
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<SessionManager> sessionManager)
        : _io_ctx(io_ctx), _userRepository(userRepository),
          _idGenerator(idGenerator), _logger(logger),
          _sessionManager(sessionManager),
          _acceptor(io_ctx, tcp::endpoint(tcp::v4(), 4125)) {
        auto networkLogger = server::logging::Logger::getNetworkLogger();
        networkLogger->info("TCP Auth Server started on port 4125");
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

        _acceptor.async_accept(
            [this, networkLogger](boost::system::error_code ec, tcp::socket socket) {
                if (ec) {
                    if (ec != boost::asio::error::operation_aborted) {
                        networkLogger->error("Accept error: {}", ec.message());
                    }
                    return;
                }
                networkLogger->info("TCP New connection accepted!");
                std::make_shared<Session>(std::move(socket), _userRepository, _idGenerator, _logger, _sessionManager)->start();
                start_accept();
            }
        );
    }
}
