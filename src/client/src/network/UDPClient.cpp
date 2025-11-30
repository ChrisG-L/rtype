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
#include <cstring>
#include <unistd.h>

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

    void UDPClient::connect(const std::string &host, std::uint16_t port)
    {
        auto logger = client::logging::Logger::getNetworkLogger();

        if (_connected) {
            logger->warn("Already connected, disconnecting...");
            disconnect();
        }

        logger->info("Connecting to {}:{}...", host, port);

        try {
            std::cout << "v4: " << udp::v4().protocol() << std::endl;
            udp::resolver resolver(_ioContext);
            udp::endpoint receiver_endpoint = *resolver.resolve(udp::v4(), host, "4124").begin();

            // asyncConnect(endpoints);
            _socket.open(udp::v4());
            std::array<char, 1> send_buf  = {{ 0 }};
            _socket.send_to(boost::asio::buffer(send_buf), receiver_endpoint);
            asyncRead(receiver_endpoint);

            _ioThread = std::jthread([this, logger]() {
                logger->debug("IO thread started");
                _ioContext.run();
                logger->debug("IO thread terminated");
            });

            logger->info("Connection initiated");
        } catch (const std::exception &e) {
            logger->error("Resolution error: {}", e.what());
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
        std::scoped_lock lock(_mutex);
        return _connected && _socket.is_open();
    }

    void UDPClient::send(const std::string &message)
    {
        if (!isConnected()) {
            return;
        }

        {
            std::scoped_lock lock(_mutex);
            _sendQueue.push(message);
        }

        boost::asio::post(_ioContext, [this]() {
            std::scoped_lock lock(_mutex);
            if (!_isWriting) {
                _isWriting = true;
                asyncWrite();
            }
        });
    }

    void UDPClient::asyncRead(udp::resolver::endpoint_type endpoint)
    {
        _socket.async_receive_from(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            endpoint,
            [this](const boost::system::error_code &error, std::size_t bytes) {
                handleRead(error, bytes);
            }
        );
    }

    void UDPClient::asyncWrite()
    {
        std::scoped_lock lock(_mutex);

        if (_sendQueue.empty()) {
            _isWriting = false;
            return;
        }

        _isWriting = true;
        std::string message = _sendQueue.front();

        boost::asio::async_write(
            _socket,
            boost::asio::buffer(message),
            [this](const boost::system::error_code &error, std::size_t) {
                handleWrite(error);
            }
        );
    }

    // void UDPClient::handleRead(const boost::system::error_code &error, std::size_t bytes)
    // {
    //     auto logger = client::logging::Logger::getNetworkLogger();
    //     if (!error) {
            
    //         _accumulator.insert(_accumulator.end(), _readBuffer, _readBuffer + bytes);
    //         if (_accumulator.size() >= Header::WIRE_SIZE) {
    //           Header head = Header::from_bytes(_accumulator.data());
    //           std::cout << "type: " << head.type << ", size: " << head.payload_size << std::endl;
    //         if (head.type == static_cast<uint16_t>(MessageType::Login))
    //             sendLoginData(_pendingUsername, _pendingPassword);
    //         else if (head.type == static_cast<uint16_t>(MessageType::Register)) {
    //             sendRegisterData(_pendingUsername, _pendingEmail, _pendingPassword);
    //         }
    //       }
    //         asyncRead();
    //     } else {
    //         if (error == boost::asio::error::eof) {
    //             logger->info("Server disconnected");
    //         } else {
    //             logger->error("Read error: {}", error.message());
    //         }

    //         if (_onError) {
    //             _onError("Erreur lecture: " + error.message());
    //         }

    //         disconnect();
    //     }
    // }

    // void UDPClient::handleWrite(const boost::system::error_code &error)
    // {
    //     auto logger = client::logging::Logger::getNetworkLogger();

    //     {
    //         std::scoped_lock lock(_mutex);
    //         if (!_sendQueue.empty()) {
    //             _sendQueue.pop();
    //         }
    //     }

    //     if (!error) {
    //         asyncWrite();
    //     } else {
    //         logger->error("Write error: {}", error.message());

    //         if (_onError) {
    //             _onError("Erreur envoi: " + error.message());
    //         }

    //         _isWriting = false;
    //         disconnect();
    //     }
    // }

}
