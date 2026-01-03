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
#include "infrastructure/tui/LogBuffer.hpp"
#include "infrastructure/tui/TerminalUI.hpp"

namespace infrastructure::adapters::in::network {
    class UDPServer;
}

namespace infrastructure::cli {

using session::SessionManager;
using adapters::in::network::UDPServer;

class ServerCLI {
public:
    ServerCLI(std::shared_ptr<SessionManager> sessionManager,
              UDPServer& udpServer,
              std::shared_ptr<tui::LogBuffer> logBuffer);
    ~ServerCLI();

    // Start the CLI in a background thread
    void start();

    // Stop the CLI (call from signal handler or main thread)
    void stop();

    // Wait for CLI thread to finish
    void join();

private:
    void runLoop();
    void executeCommand(const std::string& command);
    void printHelp();
    void printStatus();
    void listSessions();
    void kickPlayer(const std::string& args);
    void banUser(const std::string& args);
    void unbanUser(const std::string& args);
    void listBans();
    void toggleLogs(const std::string& args);
    void enterZoomMode();
    std::vector<std::string> parseArgs(const std::string& line);

    // Output helper (uses TUI if available, else stdout)
    void output(const std::string& text);

    std::shared_ptr<SessionManager> _sessionManager;
    UDPServer& _udpServer;
    std::shared_ptr<tui::LogBuffer> _logBuffer;
    std::unique_ptr<tui::TerminalUI> _terminalUI;

    std::jthread _cliThread;
    std::atomic<bool> _running{false};

    // Command handler type
    using CommandHandler = std::function<void(const std::string&)>;
    std::unordered_map<std::string, CommandHandler> _commands;
};

} // namespace infrastructure::cli

#endif /* !SERVERCLI_HPP_ */
