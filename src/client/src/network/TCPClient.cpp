/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** TCPClient
*/

#include "network/TCPClient.hpp"
#include "Protocol.hpp"
#include "core/Logger.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unistd.h>

namespace client::network
{
    TCPClient::TCPClient()
        : _socket(_ioContext), _connected(false),  _isAuthenticated(false), _isWriting(false)
    {
        client::logging::Logger::getNetworkLogger()->debug("TCPClient created");
    }

    TCPClient::~TCPClient()
    {
        disconnect();
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

        if (_connected) {
            logger->warn("Already connected, disconnecting...");
            disconnect();
        }

        logger->info("Connecting to {}:{}...", host, port);

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
            if (_onError) {
                _onError(std::string("Connexion échouée: ") + e.what());
            }
        }
    }

    void TCPClient::disconnect()
    {
        std::scoped_lock lock(_mutex);
        if (!_connected) {
            return;
        }

        auto logger = client::logging::Logger::getNetworkLogger();
        logger->info("Disconnecting...");

        _connected = false;
        _ioContext.stop();

        boost::system::error_code ec;
        _socket.shutdown(tcp::socket::shutdown_both, ec);
        _socket.close(ec);

        if (_onDisconnected) {
            _onDisconnected();
        }

        _ioContext.restart();
        _socket = tcp::socket(_ioContext);

        while (!_sendQueue.empty()) {
            _sendQueue.pop();
        }
        _isWriting = false;

        logger->info("Disconnected successfully");
    }

    bool TCPClient::isConnected() const
    {
        std::scoped_lock lock(_mutex);
        return _connected && _socket.is_open();
    }

    void TCPClient::asyncConnect(tcp::resolver::results_type endpoints)
    {
        boost::asio::async_connect(
            _socket,
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
        auto logger = client::logging::Logger::getNetworkLogger();

        if (!error) {
            {
                std::scoped_lock lock(_mutex);
                _connected = true;
            }

            logger->info("Connected successfully TCP");

            if (_onConnected) {
                _onConnected();
            }

            asyncRead();
        } else {
            logger->error("Connection failed: {}", error.message());

            if (_onError) {
                _onError("Connexion échouée: " + error.message());
            }
        }
    }

    void TCPClient::handleRead(const boost::system::error_code &error, std::size_t bytes)
    {
        auto logger = client::logging::Logger::getNetworkLogger();

        if (!error) {
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
                    // Server response to login/register - parse AuthResponse
                    if (head.payload_size >= AuthResponse::WIRE_SIZE) {
                        auto respOpt = AuthResponse::from_bytes(
                            _accumulator.data() + Header::WIRE_SIZE,
                            head.payload_size
                        );
                        if (respOpt) {
                            if (respOpt->success) {
                                _isAuthenticated = true;
                                logger->info("Authentication successful");
                                if (_onReceive) {
                                    _onReceive("authenticated");
                                }
                            } else {
                                logger->warn("Authentication failed: {} - {}", respOpt->error_code, respOpt->message);
                                if (_onError) {
                                    _onError(std::string(respOpt->message));
                                }
                            }
                        }
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
        LoginMessage login;
        std::strncpy(login.username, username.c_str(), sizeof(login.username) - 1);
        login.username[sizeof(login.username) - 1] = '\0';
        std::strncpy(login.password, password.c_str(), sizeof(login.password) - 1);
        login.password[sizeof(login.password) - 1] = '\0';

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
        std::strncpy(registerUser.username, username.c_str(), sizeof(registerUser.username) - 1);
        registerUser.username[sizeof(registerUser.username) - 1] = '\0';
        std::strncpy(registerUser.email, email.c_str(), sizeof(registerUser.email) - 1);
        registerUser.email[sizeof(registerUser.email) - 1] = '\0';
        std::strncpy(registerUser.password, password.c_str(), sizeof(registerUser.password) - 1);
        registerUser.password[sizeof(registerUser.password) - 1] = '\0';

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

}
