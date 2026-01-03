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
#include "infrastructure/tui/InteractiveOutput.hpp"
#include "application/ports/out/persistence/IUserRepository.hpp"

namespace infrastructure::adapters::in::network {
    class UDPServer;
}

namespace infrastructure::cli {

using session::SessionManager;
using adapters::in::network::UDPServer;
using application::ports::out::persistence::IUserRepository;

class ServerCLI {
public:
    ServerCLI(std::shared_ptr<SessionManager> sessionManager,
              UDPServer& udpServer,
              std::shared_ptr<tui::LogBuffer> logBuffer,
              std::shared_ptr<IUserRepository> userRepository = nullptr);
    ~ServerCLI();

    // Start the CLI in a background thread
    void start();

    // Stop the CLI (call from signal handler or main thread)
    void stop();

    // Wait for CLI thread to finish
    void join();

    // Set callback to be called when CLI requests server shutdown (exit/quit command)
    using ShutdownCallback = std::function<void()>;
    void setShutdownCallback(ShutdownCallback callback);

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
    void listUsers();
    void toggleLogs(const std::string& args);
    void enterZoomMode();
    void enterInteractMode(const std::string& args = "");
    std::vector<std::string> parseArgs(const std::string& line);

    // Interactive output generators
    tui::InteractiveOutput buildSessionsInteractiveOutput();
    tui::InteractiveOutput buildBansInteractiveOutput();
    tui::InteractiveOutput buildUsersInteractiveOutput();

    // Interact action handler
    void handleInteractAction(tui::InteractAction action, const tui::SelectableElement& element);

    // Clipboard utility
    void copyToClipboard(const std::string& text);

    // Output helper (uses TUI if available, else stdout)
    void output(const std::string& text);

    std::shared_ptr<SessionManager> _sessionManager;
    UDPServer& _udpServer;
    std::shared_ptr<tui::LogBuffer> _logBuffer;
    std::shared_ptr<IUserRepository> _userRepository;
    std::unique_ptr<tui::TerminalUI> _terminalUI;

    std::jthread _cliThread;
    std::atomic<bool> _running{false};

    // Command handler type
    using CommandHandler = std::function<void(const std::string&)>;
    std::unordered_map<std::string, CommandHandler> _commands;

    // Last interactive output (for interact command)
    tui::InteractiveOutput _lastInteractiveOutput;
    std::string _lastCommand;

    // Shutdown callback (to notify GameBootstrap to stop io_ctx)
    ShutdownCallback _shutdownCallback;
};

} // namespace infrastructure::cli

#endif /* !SERVERCLI_HPP_ */
