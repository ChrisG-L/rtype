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
#include <unordered_map>
#include <vector>

namespace infrastructure::adapters::in::network {
    // Session implementation
    Session::Session(
        tcp::socket socket,
        std::shared_ptr<MongoDBUserRepository> userRepository,
        std::unordered_map<std::string, User>& users)
    : _socket(std::move(socket)), _users(users), _isAuthenticated(false), _userRepository(userRepository)
    {
        _onAuthSuccess = [this](const User& user) { onLoginSuccess(user); };
    }

    Session::~Session()
    {
        auto logger = server::logging::Logger::getNetworkLogger();
        if (_isAuthenticated && _user.has_value()) {
            std::string username = _user->getUsername().value();
            logger->info("Session closed - removing user '{}' from active sessions", username);
            _users.erase(username);
        } else {
            logger->debug("Session closed (unauthenticated)");
        }
    }

    void Session::start() 
    {
        std::cout << "Je suis dans le start ! " << std::endl;
        do_write(MessageType::Login, "");
        do_read();
    }

    void Session::do_read() {
        auto self = shared_from_this();
        auto logger = server::logging::Logger::getNetworkLogger();

        _socket.async_read_some(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            [this, self, logger](boost::system::error_code ec, std::size_t bytes) {
                if (!ec) {
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

    void Session::handle_command(const Header& head) {
        using infrastructure::adapters::in::network::execute::Execute;
        auto logger = server::logging::Logger::getNetworkLogger();

        Command cmd = {.type = head.type, .buf = std::vector<uint8_t>(_accumulator.begin() + Header::WIRE_SIZE, _accumulator.begin() + Header::WIRE_SIZE + head.payload_size)};

        // Determine response type based on request type
        MessageType responseType = (head.type == static_cast<uint16_t>(MessageType::Login))
            ? MessageType::LoginAck
            : MessageType::RegisterAck;

        try {
            Execute execute(cmd, _userRepository, _users, _onAuthSuccess);

            // Check if authentication succeeded
            if (_isAuthenticated) {
                logger->info("Authentication successful");
                AuthResponse resp;
                resp.success = true;
                std::strncpy(resp.error_code, "", MAX_ERROR_CODE_LEN);
                std::strncpy(resp.message, "Authentication successful", MAX_ERROR_MSG_LEN);
                do_write_auth_response(responseType, resp);
            } else {
                logger->info("Authentication failed - invalid credentials");
                AuthResponse resp;
                resp.success = false;
                std::strncpy(resp.error_code, "INVALID_CREDENTIALS", MAX_ERROR_CODE_LEN);
                std::strncpy(resp.message, "Invalid username or password", MAX_ERROR_MSG_LEN);
                do_write_auth_response(responseType, resp);
            }
        } catch (const domain::exceptions::user::UsernameAlreadyExistsException& e) {
            logger->warn("Username already exists: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "USERNAME_EXISTS", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(MessageType::RegisterAck, resp);
        } catch (const domain::exceptions::user::EmailAlreadyExistsException& e) {
            logger->warn("Email already exists: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "EMAIL_EXISTS", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(MessageType::RegisterAck, resp);
        } catch (const domain::exceptions::user::UserAlreadyLoggedException& e) {
            logger->warn("User already logged: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "USER_ALREADY_LOGGED", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(responseType, resp);
        } catch (const domain::exceptions::user::UsernameException& e) {
            logger->warn("Username validation error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "INVALID_USERNAME", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(responseType, resp);
        } catch (const domain::exceptions::user::EmailException& e) {
            logger->warn("Email validation error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "INVALID_EMAIL", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(MessageType::RegisterAck, resp);
        } catch (const domain::exceptions::user::PasswordException& e) {
            logger->warn("Password validation error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "INVALID_PASSWORD", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(responseType, resp);
        } catch (const domain::exceptions::DomainException& e) {
            logger->error("Domain error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "DOMAIN_ERROR", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(responseType, resp);
        } catch (const std::exception& e) {
            logger->error("Server error: {}", e.what());
            AuthResponse resp;
            resp.success = false;
            std::strncpy(resp.error_code, "SERVER_ERROR", MAX_ERROR_CODE_LEN);
            std::strncpy(resp.message, e.what(), MAX_ERROR_MSG_LEN);
            resp.message[MAX_ERROR_MSG_LEN - 1] = '\0';
            do_write_auth_response(responseType, resp);
        }
    }

    // TCPAuthServer implementation
    TCPAuthServer::TCPAuthServer(boost::asio::io_context& io_ctx, std::shared_ptr<MongoDBUserRepository> userRepository)
        : _io_ctx(io_ctx), _userRepository(userRepository) ,
        _acceptor(io_ctx, tcp::endpoint(tcp::v4(), 4125)) {
        auto logger = server::logging::Logger::getNetworkLogger();
        logger->info("TCP Auth Server started on port 4125");
    }

    void TCPAuthServer::start() {
        start_accept();
    }

    void TCPAuthServer::run() {
        _io_ctx.run();
    }

    void TCPAuthServer::start_accept() {
        auto logger = server::logging::Logger::getNetworkLogger();

        _acceptor.async_accept(
            [this, logger](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    logger->info("TCP New connection accepted!");
                    std::make_shared<Session>(std::move(socket), _userRepository, users)->start();
                }
                start_accept();
            }
        );
    }
}
