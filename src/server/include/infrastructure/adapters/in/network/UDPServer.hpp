/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** UDPServer
*/

#ifndef UDPSERVER_HPP_
#define UDPSERVER_HPP_

#include <array>
#include <boost/asio.hpp>
#include "Protocol.hpp"
#include "infrastructure/game/GameWorld.hpp"
#include <memory>


namespace infrastructure::adapters::in::network {
    using boost::asio::ip::udp;

    class UDPServer {
        private:
            boost::asio::io_context& _io_ctx;
            udp::socket _socket;
            udp::endpoint _remote_endpoint;
            game::GameWorld _gameWorld;
            boost::asio::steady_timer _broadcastTimer;

            char _readBuffer[BUFFER_SIZE];

            void sendTo(const udp::endpoint& endpoint, const void* data, size_t size);
            void sendPlayerJoin(const udp::endpoint& endpoint, uint8_t playerId);
            void sendPlayerLeave(uint8_t playerId);
            void broadcastSnapshot();
            void scheduleBroadcast();

            void do_read();
            void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);

        public:
            explicit UDPServer(boost::asio::io_context& io_ctx);
            void start();
            void run();
            void stop();
    };
}
#endif /* !UDPSERVER_HPP_ */
