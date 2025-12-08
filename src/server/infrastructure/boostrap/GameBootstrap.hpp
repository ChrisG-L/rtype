/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** GameBootstrap
*/

#ifndef GAMEBOOTSTRAP_HPP_
#define GAMEBOOTSTRAP_HPP_

#include "domain/value_objects/user/Password.hpp"

#include "infrastructure/adapters/out/persistence/MongoDBUserRepository.hpp"
#include "infrastructure/adapters/in/network/UDPServer.hpp"
#include "infrastructure/adapters/in/network/TCPServer.hpp"

#include <iostream>
#include <memory>
#include <cstdlib>
#include <stdexcept>

namespace infrastructure::boostrap {
    using adapters::out::persistence::MongoDBConfiguration;
    using adapters::out::persistence::MongoDBUserRepository;

    class GameBootstrap {
        private:
            std::shared_ptr<MongoDBConfiguration> _mongoDB;

            void mongodb() {
                std::cout << "=== Démarrage de la base de donnée mongoDB ===" << std::endl;
                const char* mongoURI = std::getenv("MONGODB_URI");
                if (!mongoURI) {
                    throw std::runtime_error("MONGODB_URI environment variable not set");
                }
                DBConfig dbConfig = {.connexionString = std::string(mongoURI), .dbName = "rtype"};
                _mongoDB = std::make_shared<MongoDBConfiguration>(dbConfig);
            };

            void server() {
                using adapters::in::network::UDPServer;
                using adapters::in::network::TCPServer;
                std::cout << "=== Démarrage du serveur R-Type ===" << std::endl;

                auto userRepository = std::make_shared<MongoDBUserRepository>(_mongoDB);

                boost::asio::io_context io_ctx;

                TCPServer tcpServer(io_ctx, userRepository);
                UDPServer udpServer(io_ctx);

                tcpServer.start();
                udpServer.start();

                std::cout << "Serveur UDP prêt. En attente de connexions..." << std::endl;
                std::cout << "Serveur TCP prêt. En attente de connexions..." << std::endl;

                io_ctx.run();
            }

        public:
            GameBootstrap() = default;

            void launch() {
                mongodb();
                std::cout << std::endl;
                server();

            };

    };
}

#endif /* !GAMEBOOTSTRAP_HPP_ */
