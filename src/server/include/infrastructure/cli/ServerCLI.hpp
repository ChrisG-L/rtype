/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** ServerCLI - Interactive command-line interface for server administration
*/

#ifndef SERVERCLI_HPP_
#define SERVERCLI_HPP_

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <memory>
#include "infrastructure/session/SessionManager.hpp"

namespace infrastructure::adapters::in::network {
    class UDPServer;
}

namespace infrastructure::cli {

using session::SessionManager;
using adapters::in::network::UDPServer;

class ServerCLI {
public:
    ServerCLI(std::shared_ptr<SessionManager> sessionManager, UDPServer& udpServer);
    ~ServerCLI();

    // Start the CLI in a background thread
    void start();

    // Stop the CLI (call from signal handler or main thread)
    void stop();

    // Wait for CLI thread to finish
    void join();

private:
    void runLoop();
    void printHelp();
    void printStatus();
    void listSessions();
    void kickPlayer(const std::string& args);
    void banUser(const std::string& args);
    void unbanUser(const std::string& args);
    void listBans();
    void toggleLogs(const std::string& args);
    std::vector<std::string> parseArgs(const std::string& line);

    std::shared_ptr<SessionManager> _sessionManager;
    UDPServer& _udpServer;

    std::jthread _cliThread;
    std::atomic<bool> _running{false};

    // Command handler type
    using CommandHandler = std::function<void(const std::string&)>;
    std::unordered_map<std::string, CommandHandler> _commands;
};

} // namespace infrastructure::cli

#endif /* !SERVERCLI_HPP_ */
