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
#include "infrastructure/adapters/out/persistence/MongoDBUserSettingsRepository.hpp"
#include "infrastructure/adapters/out/persistence/MongoDBChatMessageRepository.hpp"
#include "infrastructure/adapters/out/MongoIdGenerator.hpp"
#include "infrastructure/adapters/out/SpdLogAdapter.hpp"
#include "infrastructure/configuration/DBConfig.hpp"
#include "infrastructure/session/SessionManager.hpp"
#include "infrastructure/room/RoomManager.hpp"
#include "infrastructure/cli/ServerCLI.hpp"
#include "infrastructure/tui/LogBuffer.hpp"
#include "infrastructure/logging/Logger.hpp"

#include <memory>
#include <cstdlib>
#include <csignal>
#include <vector>
#include <thread>

namespace infrastructure::bootstrap {

    class GameBootstrap {
        private:
            void server() {
                using adapters::in::network::UDPServer;
                using adapters::in::network::TCPAuthServer;
                using adapters::out::persistence::MongoDBConfiguration;
                using adapters::out::persistence::MongoDBUserRepository;
                using adapters::out::persistence::MongoDBUserSettingsRepository;
                using adapters::out::persistence::MongoDBChatMessageRepository;
                using adapters::out::MongoIdGenerator;
                using adapters::out::SpdLogAdapter;
                using session::SessionManager;
                using room::RoomManager;

                // Create LogBuffer first (before Logger init)
                auto logBuffer = std::make_shared<tui::LogBuffer>();

                // Initialize Logger with TUI mode
                server::logging::Logger::initWithTUI(logBuffer);

                auto mainLogger = server::logging::Logger::getMainLogger();
                mainLogger->info("=== Démarrage du serveur R-Type ===");

                boost::asio::io_context io_ctx;

                // Work guard keeps io_context running even when there's no work
                // This is needed because threads may start before async operations are posted
                auto workGuard = boost::asio::make_work_guard(io_ctx);

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
                auto userSettingsRepo = std::make_shared<MongoDBUserSettingsRepository>(mongoConfig);
                auto chatMessageRepo = std::make_shared<MongoDBChatMessageRepository>(mongoConfig);

                // Create adapters for ports
                auto idGenerator = std::make_shared<MongoIdGenerator>();
                auto logger = std::make_shared<SpdLogAdapter>();

                // Create shared SessionManager for TCP and UDP servers
                auto sessionManager = std::make_shared<SessionManager>();

                // Create shared RoomManager for room/lobby management (with chat persistence)
                auto roomManager = std::make_shared<RoomManager>(chatMessageRepo);

                // Start TCP Auth Server on port 4125
                TCPAuthServer tcpAuthServer(io_ctx, userRepo, userSettingsRepo, idGenerator, logger, sessionManager, roomManager);
                tcpAuthServer.start();

                // Start UDP Game Server on port 4124 (shares SessionManager with TCP)
                UDPServer udpServer(io_ctx, sessionManager);
                udpServer.start();

                mainLogger->info("Serveur UDP prêt. En attente de connexions...");
                mainLogger->info("Appuyez sur Ctrl+C pour arrêter le serveur proprement.");

                // Start CLI with TUI support
                cli::ServerCLI serverCLI(sessionManager, udpServer, logBuffer, userRepo, roomManager);

                // Set shutdown callback so CLI can stop the server via exit/quit commands
                serverCLI.setShutdownCallback([&]() {
                    mainLogger->info("CLI requested shutdown");
                    udpServer.stop();
                    tcpAuthServer.stop();
                    workGuard.reset();  // Allow io_context to finish when no more work
                    io_ctx.stop();
                });

                serverCLI.start();

                boost::asio::signal_set signals(io_ctx, SIGINT, SIGTERM);
                signals.async_wait([&](const boost::system::error_code&, int signum) {
                    mainLogger->info("Signal reçu ({}), arrêt du serveur...", signum);
                    serverCLI.stop();
                    udpServer.stop();
                    tcpAuthServer.stop();
                    workGuard.reset();  // Allow io_context to finish when no more work
                    io_ctx.stop();
                });

                // Thread pool for parallel execution of io_context handlers
                constexpr size_t THREAD_POOL_SIZE = 4;
                std::vector<std::jthread> threadPool;
                threadPool.reserve(THREAD_POOL_SIZE);

                mainLogger->info("Starting thread pool with {} workers", THREAD_POOL_SIZE);

                for (size_t i = 0; i < THREAD_POOL_SIZE; ++i) {
                    threadPool.emplace_back([&io_ctx, &mainLogger, i]() {
                        mainLogger->debug("Thread pool worker {} started", i);
                        io_ctx.run();
                        mainLogger->debug("Thread pool worker {} stopped", i);
                    });
                }

                // std::jthread automatically joins on destruction when threadPool goes out of scope

                // Wait for CLI to finish
                serverCLI.join();

                // Note: Logger::shutdown() is called in main() after all components are destroyed
            }

        public:
            GameBootstrap() = default;

            void launch() {
                server();
            };

    };
}

#endif /* !GAMEBOOTSTRAP_HPP_ */
