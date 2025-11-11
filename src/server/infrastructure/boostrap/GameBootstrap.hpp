/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** GameBootstrap
*/

#ifndef GAMEBOOTSTRAP_HPP_
#define GAMEBOOTSTRAP_HPP_

#include "infrastructure/adapters/out/persistence/MongoDBConfiguration.hpp"
#include "infrastructure/adapters/in/network/UDPServer.hpp"
#include "infrastructure/adapters/in/network/TCPServer.hpp"

#include <iostream>
#include <memory>

namespace infrastructure::boostrap {
    using adapters::out::persistence::MongoDBConfiguration;

    class GameBootstrap {
        private:
            std::unique_ptr<MongoDBConfiguration> _mongoDB;

            void mongodb() {
                std::cout << "=== Démarrage de la base de donnée mongoDB ===" << std::endl;
                std::string mongoURI = "mongodb+srv://dbUser:root@rtypehome.qxzb27g.mongodb.net/";
                DBConfig dbConfig = {.connexionString = mongoURI, .dbName = "rtype"};
                _mongoDB = std::make_unique<MongoDBConfiguration>(dbConfig);
            };

            void server() {
                using adapters::in::network::UDPServer;
                using adapters::in::network::TCPServer;
                std::cout << "=== Démarrage du serveur R-Type ===" << std::endl;

                // 1. Créer le contexte IO (boucle événementielle Boost.Asio)
                boost::asio::io_context io_ctx;

                TCPServer tcpServer(io_ctx);

                tcpServer.start(io_ctx);

                std::cout << "Serveur prêt. En attente de connexions..." << std::endl;

                tcpServer.run();

                // // 2. Créer le serveur UDP
                // UDPServer udpServer(io_ctx);

                // // 3. Démarrer l'écoute
                // udpServer.start(io_ctx);

                // std::cout << "Serveur prêt. En attente de connexions..." << std::endl;

                // // 4. Lancer la boucle événementielle (bloquant)
                // udpServer.run();
            }

        public:
            GameBootstrap() {};

            void launch() {
                mongodb();
                std::cout << std::endl;
                server();

            };

    };
}

#endif /* !GAMEBOOTSTRAP_HPP_ */
