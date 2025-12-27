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
    static constexpr int HEARTBEAT_INTERVAL_MS = 1000;
    static constexpr int HEARTBEAT_TIMEOUT_MS = 5000;

    TCPClient::TCPClient()
        : _socket(_ioContext), _heartbeatTimer(_ioContext), _isAuthenticated(false), _isWriting(false)
    {
        client::logging::Logger::getNetworkLogger()->debug("TCPClient created");
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

        if (_connected.load()) {
            logger->warn("Already connected, disconnecting...");
            disconnect();
        }

        // Wait for previous IO thread to finish (if any)
        if (_ioThread.joinable()) {
            _ioThread.join();
        }

        logger->info("Connecting to {}:{}...", host, port);

        _disconnecting.store(false);

        // Reset io_context for reuse
        _ioContext.restart();

        // Recreate socket if closed
        if (!_socket.is_open()) {
            _socket = tcp::socket(_ioContext);
        }

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
        // Prevent multiple disconnect calls
        if (_disconnecting.exchange(true)) {
            return;
        }

        if (!_connected.load()) {
            _disconnecting.store(false);
            return;
        }

        auto logger = client::logging::Logger::getNetworkLogger();
        logger->info("Disconnecting TCP...");

        _connected.store(false);

        _heartbeatTimer.cancel();

        _ioContext.stop();

        boost::system::error_code ec;
        _socket.shutdown(tcp::socket::shutdown_both, ec);
        _socket.close(ec);

        _accumulator.clear();

        _eventQueue.push(TCPDisconnectedEvent{});
        if (_onDisconnected) {
            _onDisconnected();
        }

        _ioContext.restart();
        _socket = tcp::socket(_ioContext);

        while (!_sendQueue.empty()) {
            _sendQueue.pop();
        }
        _isWriting = false;

        logger->info("TCP disconnected successfully");
    }

    bool TCPClient::isConnected() const
    {
        return _connected.load() && _socket.is_open();
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
            _connected.store(true);

            logger->info("Connected successfully TCP");

            {
                std::lock_guard<std::mutex> lock(_heartbeatMutex);
                _lastServerResponse = std::chrono::steady_clock::now();
            }
            sendHeartbeat();
            scheduleHeartbeat();

            _eventQueue.push(TCPConnectedEvent{});
            if (_onConnected) {
                _onConnected();
            }

            asyncRead();
        } else {
            logger->error("Connection failed: {}", error.message());

            _eventQueue.push(TCPErrorEvent{"Connexion echouee: " + error.message()});
            if (_onError) {
                _onError("Connexion echouee: " + error.message());
            }
        }
    }

    void TCPClient::handleRead(const boost::system::error_code &error, std::size_t bytes)
    {
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
                                _eventQueue.push(TCPAuthSuccessEvent{});
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
                    // HeartBeat response - already updated _lastServerResponse above
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

    void TCPClient::sendHeartbeat() {
        if (!isConnected()) {
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
            if (ec || !_connected.load()) {
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
                logger->warn("TCP Server heartbeat timeout ({}ms)", elapsed);

                _eventQueue.push(TCPErrorEvent{"Timeout: Serveur d'authentification injoignable"});
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

}
