/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** UDPClient
*/

#include "network/UDPClient.hpp"
#include "Protocol.hpp"
#include "core/Logger.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <vector>

namespace client::network
{
    UDPClient::UDPClient()
        : _socket(_ioContext), _connected(false), _isWriting(false)
    {
        client::logging::Logger::getNetworkLogger()->debug("UDPClient created");
    }

    UDPClient::~UDPClient()
    {
        disconnect();
    }

    void UDPClient::setOnConnected(const OnConnectedCallback& callback)
    {
        _onConnected = callback;
    }

    void UDPClient::setOnDisconnected(const OnDisconnectedCallback& callback)
    {
        _onDisconnected = callback;
    }

    void UDPClient::setOnReceive(const OnReceiveCallback& callback)
    {
        _onReceive = callback;
    }

    void UDPClient::setOnError(const OnErrorCallback& callback)
    {
        _onError = callback;
    }

    void UDPClient::connect(std::shared_ptr<client::network::TCPClient> tcpClient, const std::string &host, std::uint16_t port)
    {
        auto logger = client::logging::Logger::getNetworkLogger();
        _tcpClient = tcpClient;
        if (_connected) {
            logger->warn("Already connected, disconnecting...");
            disconnect();
        }
        logger->info("Connecting to {}:{}...", host, port);
        
        try {
        
            udp::resolver resolver(_ioContext);
            auto results = resolver.resolve(host, std::to_string(port));
            _endpoint = *results.begin();
            
            _socket.open(udp::v4());
            
            _connected = true;
            
            asyncReceiveFrom();
            
            _ioThread = std::jthread([this, logger]() {
                logger->debug("IO thread started");
                _ioContext.run();
                logger->debug("IO thread terminated");
            });
            
            logger->info("Connected to server UDP at {}:{}", 
                        _endpoint.address().to_string(), _endpoint.port());
                        
        } catch (const std::exception &e) {
            logger->error("Connection error: {}", e.what());
            if (_onError) {
                _onError(std::string("Connexion échouée: ") + e.what());
            }
        }
    }

    void UDPClient::disconnect()
    {
        // std::scoped_lock lock(_mutex);
        // if (!_connected) {
        //     return;
        // }

        // auto logger = client::logging::Logger::getNetworkLogger();
        // logger->info("Disconnecting...");

        // _connected = false;
        // _ioContext.stop();

        // boost::system::error_code ec;
        // _socket.shutdown(udp::socket::shutdown_both, ec);
        // _socket.close(ec);

        // if (_onDisconnected) {
        //     _onDisconnected();
        // }

        // _ioContext.restart();
        // _socket = udp::socket(_ioContext);

        // {
        //     std::scoped_lock lock(_mutex);
        //     while (!_sendQueue.empty()) {
        //         _sendQueue.pop();
        //     }
        //     _isWriting = false;
        // }

        // logger->info("Disconnected successfully");
    }

    bool UDPClient::isConnected() const
    {
        return _connected && _socket.is_open();
    }

    bool UDPClient::isAuthenticated() const
    {
        std::scoped_lock lock(_mutex);
        return isConnected() && _tcpClient->isAuthenticated();
    }

    void UDPClient::asyncReceiveFrom()
    {
        if (!isAuthenticated()) {
            std::cout << "User not authenticated!" << std::endl;
            return;
        }
        _socket.async_receive_from(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            _endpoint,
            [this](const boost::system::error_code &error, std::size_t bytes) {
                handleRead(error, bytes);
            }
        );
    }
    
    void UDPClient::asyncSendTo(std::shared_ptr<std::vector<uint8_t>>& buf, size_t totalSize) {
        if (!isAuthenticated()) {
            std::cout << "User not authenticated!" << std::endl;
            return;
        }
        _socket.async_send_to(
            boost::asio::buffer(buf->data(), totalSize),
            _endpoint,
            [this, buf](const boost::system::error_code &error, std::size_t) {
                // handleWrite(error);
                if (error) {
                    std::cout << "WRITE UDP ERROR: " << error << std::endl;
                }
            }
        );
    }

    void UDPClient::handleRead(const boost::system::error_code &error, std::size_t bytes)
    {
        auto logger = client::logging::Logger::getNetworkLogger();
        if (!error && bytes > 0) {
            if (bytes >= UDPHeader::WIRE_SIZE) {
                UDPHeader head = UDPHeader::from_bytes(_readBuffer);

                if (head.type == static_cast<uint16_t>(MessageType::Snapshop)) {
                    std::cout << "snapShot!" << std::endl;
                }

                    // if (head.type == static_cast<uint16_t>(MessageType::Login))
                    //     sendLoginData(_pendingUsername, _pendingPassword);
                    // else if (head.type == static_cast<uint16_t>(MessageType::Register)) {
                    //     sendRegisterData(_pendingUsername, _pendingEmail, _pendingPassword);
                    // }
                // }
                asyncReceiveFrom();
            }
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


    void UDPClient::movePlayer(uint16_t x, uint16_t y) {
        MovePlayer movePlayer = {.x = x, .y = y};

        UDPHeader head = {
            .type = static_cast<uint16_t>(MessageType::MovePlayer),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };

        const size_t totalSize = UDPHeader::WIRE_SIZE + MovePlayer::WIRE_SIZE;

        auto buf = std::make_shared<std::vector<uint8_t>>(totalSize);

        head.to_bytes(buf->data());
        movePlayer.to_bytes(buf->data() + UDPHeader::WIRE_SIZE);
        asyncSendTo(buf, totalSize);
    }

}
