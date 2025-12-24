/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** GameBootstrap
*/

#ifndef GAMEBOOTSTRAP_HPP_
#define GAMEBOOTSTRAP_HPP_

#include "infrastructure/adapters/in/network/UDPServer.hpp"
#include "infrastructure/adapters/in/network/TCPAuthServer.hpp"
#include "infrastructure/adapters/out/persistence/MongoDBConfiguration.hpp"
#include "infrastructure/adapters/out/persistence/MongoDBUserRepository.hpp"
#include "infrastructure/adapters/out/MongoIdGenerator.hpp"
#include "infrastructure/adapters/out/SpdLogAdapter.hpp"
#include "infrastructure/configuration/DBConfig.hpp"

#include <iostream>
#include <memory>
#include <cstdlib>
#include <csignal>

namespace infrastructure::bootstrap {

    class GameBootstrap {
        private:
            void server() {
                using adapters::in::network::UDPServer;
                using adapters::in::network::TCPAuthServer;
                using adapters::out::persistence::MongoDBConfiguration;
                using adapters::out::persistence::MongoDBUserRepository;
                using adapters::out::MongoIdGenerator;
                using adapters::out::SpdLogAdapter;

                std::cout << "=== Démarrage du serveur R-Type ===" << std::endl;

                boost::asio::io_context io_ctx;

                // Get MongoDB URI from environment variable or use default
                const char* mongoUri = std::getenv("MONGODB_URI");
                const char* mongoDb = std::getenv("MONGODB_DB");

                DBConfig dbConfig{
                    .connexionString = mongoUri ? mongoUri : "mongodb://localhost:8089",
                    .dbName = mongoDb ? mongoDb : "rtype",
                    .minPoolSize = 1,
                    .maxPoolSize = 10
                };

                // Initialize MongoDB
                auto mongoConfig = std::make_shared<MongoDBConfiguration>(dbConfig);
                auto userRepo = std::make_shared<MongoDBUserRepository>(mongoConfig);

                // Create adapters for ports
                auto idGenerator = std::make_shared<MongoIdGenerator>();
                auto logger = std::make_shared<SpdLogAdapter>();

                // Start TCP Auth Server on port 4125
                TCPAuthServer tcpAuthServer(io_ctx, userRepo, idGenerator, logger);
                tcpAuthServer.start();

                // Start UDP Game Server on port 4124
                UDPServer udpServer(io_ctx);
                udpServer.start();

                std::cout << "Serveur UDP prêt. En attente de connexions..." << std::endl;
                std::cout << "Appuyez sur Ctrl+C pour arrêter le serveur proprement." << std::endl;

                boost::asio::signal_set signals(io_ctx, SIGINT, SIGTERM);
                signals.async_wait([&](const boost::system::error_code&, int signum) {
                    std::cout << "\nSignal reçu (" << signum << "), arrêt du serveur..." << std::endl;
                    udpServer.stop();
                    tcpAuthServer.stop();
                    io_ctx.stop();
                });

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
