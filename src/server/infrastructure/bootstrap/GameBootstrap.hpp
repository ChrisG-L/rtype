/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** GameBootstrap
*/

#ifndef GAMEBOOTSTRAP_HPP_
#define GAMEBOOTSTRAP_HPP_

#include "infrastructure/adapters/in/network/UDPServer.hpp"

#include <iostream>
#include <memory>

namespace infrastructure::bootstrap {

    class GameBootstrap {
        private:
            void server() {
                using adapters::in::network::UDPServer;
                std::cout << "=== Démarrage du serveur R-Type ===" << std::endl;

                boost::asio::io_context io_ctx;

                UDPServer udpServer(io_ctx);

                udpServer.start();

                std::cout << "Serveur UDP prêt. En attente de connexions..." << std::endl;

                io_ctx.run();
            }

        public:
            GameBootstrap() = default;

            void launch() {
                server();
            };

    };
}

#endif /* !GAMEBOOTSTRAP_HPP_ */
