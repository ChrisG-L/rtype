/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** ServerCLI implementation
*/

#include "infrastructure/cli/ServerCLI.hpp"
#include "infrastructure/adapters/in/network/UDPServer.hpp"
#include "infrastructure/logging/Logger.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

#ifndef _WIN32
    #include <poll.h>
    #include <unistd.h>
#else
    #include <conio.h>
    #include <windows.h>
#endif

namespace infrastructure::cli {

ServerCLI::ServerCLI(std::shared_ptr<SessionManager> sessionManager, UDPServer& udpServer)
    : _sessionManager(sessionManager), _udpServer(udpServer)
{
    // Register commands
    _commands["help"] = [this](const std::string&) { printHelp(); };
    _commands["status"] = [this](const std::string&) { printStatus(); };
    _commands["sessions"] = [this](const std::string&) { listSessions(); };
    _commands["kick"] = [this](const std::string& args) { kickPlayer(args); };
    _commands["ban"] = [this](const std::string& args) { banUser(args); };
    _commands["unban"] = [this](const std::string& args) { unbanUser(args); };
    _commands["bans"] = [this](const std::string&) { listBans(); };
    _commands["logs"] = [this](const std::string& args) { toggleLogs(args); };
    _commands["quit"] = [this](const std::string&) { stop(); };
    _commands["exit"] = [this](const std::string&) { stop(); };
}

ServerCLI::~ServerCLI() {
    stop();
    if (_cliThread.joinable()) {
        _cliThread.join();
    }
}

void ServerCLI::start() {
    _running = true;
    _cliThread = std::jthread([this](std::stop_token stoken) {
        runLoop();
    });
}

void ServerCLI::stop() {
    _running = false;
    std::cout << "\n[CLI] Stopping..." << std::endl;
}

void ServerCLI::join() {
    if (_cliThread.joinable()) {
        _cliThread.join();
    }
}

// Check if stdin has data available (non-blocking)
static bool stdinHasData(int timeoutMs) {
#ifndef _WIN32
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, timeoutMs);
    return ret > 0 && (pfd.revents & POLLIN);
#else
    // Windows: check if console has input
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD events = 0;
    if (GetNumberOfConsoleInputEvents(hStdin, &events) && events > 0) {
        return true;
    }
    Sleep(timeoutMs);
    return _kbhit() != 0;
#endif
}

void ServerCLI::runLoop() {
    std::cout << "\n╔══════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     R-Type Server CLI - Type 'help' for commands  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════╝\n" << std::endl;

    bool needPrompt = true;

    while (_running) {
        if (needPrompt) {
            std::cout << "rtype> " << std::flush;
            needPrompt = false;
        }

        // Poll stdin with 100ms timeout to allow checking _running flag
        if (!stdinHasData(100)) {
            continue;
        }

        std::string line;
        if (!std::getline(std::cin, line)) {
            // EOF (Ctrl+D) or input error
            if (std::cin.eof()) {
                std::cout << "\n[CLI] EOF received, use Ctrl+C to stop server." << std::endl;
                std::cin.clear();
                needPrompt = true;
                continue;
            }
            break;
        }

        needPrompt = true;

        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        // Parse command and arguments
        auto args = parseArgs(line);
        if (args.empty()) {
            continue;
        }

        std::string command = args[0];
        std::string argStr;
        if (args.size() > 1) {
            std::ostringstream oss;
            for (size_t i = 1; i < args.size(); ++i) {
                if (i > 1) oss << " ";
                oss << args[i];
            }
            argStr = oss.str();
        }

        // Find and execute command
        auto it = _commands.find(command);
        if (it != _commands.end()) {
            try {
                it->second(argStr);
            } catch (const std::exception& e) {
                std::cout << "[CLI] Error: " << e.what() << std::endl;
            }
        } else {
            std::cout << "[CLI] Unknown command: " << command << ". Type 'help' for available commands." << std::endl;
        }
    }
}

std::vector<std::string> ServerCLI::parseArgs(const std::string& line) {
    std::vector<std::string> args;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        args.push_back(token);
    }
    return args;
}

void ServerCLI::printHelp() {
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    AVAILABLE COMMANDS                         ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ help                  - Show this help message               ║" << std::endl;
    std::cout << "║ status                - Show server status                   ║" << std::endl;
    std::cout << "║ sessions              - List all active sessions             ║" << std::endl;
    std::cout << "║ kick <player_id>      - Kick a player by their in-game ID   ║" << std::endl;
    std::cout << "║ ban <email>           - Ban a user permanently               ║" << std::endl;
    std::cout << "║ unban <email>         - Unban a user                         ║" << std::endl;
    std::cout << "║ bans                  - List all banned users                ║" << std::endl;
    std::cout << "║ logs <on|off>         - Enable/disable server logs           ║" << std::endl;
    std::cout << "║ quit/exit             - Stop the CLI (use Ctrl+C to stop    ║" << std::endl;
    std::cout << "║                         the server)                          ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n" << std::endl;
}

void ServerCLI::printStatus() {
    size_t sessionCount = _sessionManager->getSessionCount();
    size_t playerCount = _udpServer.getPlayerCount();

    std::cout << "\n╔══════════════════════════════════════╗" << std::endl;
    std::cout << "║          SERVER STATUS               ║" << std::endl;
    std::cout << "╠══════════════════════════════════════╣" << std::endl;
    std::cout << "║ Active Sessions: " << std::setw(4) << sessionCount << "                 ║" << std::endl;
    std::cout << "║ Players in Game: " << std::setw(4) << playerCount << "                 ║" << std::endl;
    std::cout << "╚══════════════════════════════════════╝\n" << std::endl;
}

void ServerCLI::listSessions() {
    auto sessions = _sessionManager->getAllSessions();

    if (sessions.empty()) {
        std::cout << "\n[CLI] No active sessions.\n" << std::endl;
        return;
    }

    std::cout << "\n╔══════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                ACTIVE SESSIONS                                    ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ " << std::left << std::setw(25) << "Email"
              << std::setw(15) << "Display Name"
              << std::setw(10) << "Status"
              << std::setw(10) << "Player ID"
              << std::setw(22) << "Endpoint" << " ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════════════════════════╣" << std::endl;

    for (const auto& session : sessions) {
        std::string statusStr;
        switch (session.status) {
            case session::Session::Status::Pending: statusStr = "Pending"; break;
            case session::Session::Status::Active: statusStr = "Active"; break;
            case session::Session::Status::Expired: statusStr = "Expired"; break;
        }

        std::string playerIdStr = session.playerId ? std::to_string(*session.playerId) : "-";
        std::string endpointStr = session.udpEndpoint.empty() ? "-" : session.udpEndpoint;

        // Truncate long strings
        std::string email = session.email.length() > 24 ? session.email.substr(0, 21) + "..." : session.email;
        std::string displayName = session.displayName.length() > 14 ? session.displayName.substr(0, 11) + "..." : session.displayName;
        if (endpointStr.length() > 21) {
            endpointStr = endpointStr.substr(0, 18) + "...";
        }

        std::cout << "║ " << std::left << std::setw(25) << email
                  << std::setw(15) << displayName
                  << std::setw(10) << statusStr
                  << std::setw(10) << playerIdStr
                  << std::setw(22) << endpointStr << " ║" << std::endl;
    }

    std::cout << "╚══════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;
}

void ServerCLI::kickPlayer(const std::string& args) {
    if (args.empty()) {
        std::cout << "[CLI] Usage: kick <player_id>" << std::endl;
        return;
    }

    try {
        int playerId = std::stoi(args);
        if (playerId < 0 || playerId > 255) {
            std::cout << "[CLI] Invalid player ID. Must be 0-255." << std::endl;
            return;
        }
        _udpServer.kickPlayer(static_cast<uint8_t>(playerId));
    } catch (const std::exception& e) {
        std::cout << "[CLI] Invalid player ID: " << args << std::endl;
    }
}

void ServerCLI::toggleLogs(const std::string& args) {
    if (args.empty()) {
        // Toggle current state
        bool currentState = server::logging::Logger::isEnabled();
        server::logging::Logger::setEnabled(!currentState);
        std::cout << "[CLI] Logs " << (!currentState ? "enabled" : "disabled") << std::endl;
        return;
    }

    if (args == "on" || args == "1" || args == "true") {
        server::logging::Logger::setEnabled(true);
        std::cout << "[CLI] Logs enabled" << std::endl;
    } else if (args == "off" || args == "0" || args == "false") {
        server::logging::Logger::setEnabled(false);
        std::cout << "[CLI] Logs disabled" << std::endl;
    } else {
        std::cout << "[CLI] Usage: logs <on|off>" << std::endl;
    }
}

void ServerCLI::banUser(const std::string& args) {
    if (args.empty()) {
        std::cout << "[CLI] Usage: ban <email>" << std::endl;
        return;
    }

    if (_sessionManager->isBanned(args)) {
        std::cout << "[CLI] User '" << args << "' is already banned." << std::endl;
        return;
    }

    _sessionManager->banUser(args);
    std::cout << "[CLI] User '" << args << "' has been banned." << std::endl;
}

void ServerCLI::unbanUser(const std::string& args) {
    if (args.empty()) {
        std::cout << "[CLI] Usage: unban <email>" << std::endl;
        return;
    }

    if (!_sessionManager->isBanned(args)) {
        std::cout << "[CLI] User '" << args << "' is not banned." << std::endl;
        return;
    }

    _sessionManager->unbanUser(args);
    std::cout << "[CLI] User '" << args << "' has been unbanned." << std::endl;
}

void ServerCLI::listBans() {
    auto bannedUsers = _sessionManager->getBannedUsers();

    if (bannedUsers.empty()) {
        std::cout << "\n[CLI] No banned users.\n" << std::endl;
        return;
    }

    std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                       BANNED USERS                            ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;

    for (const auto& email : bannedUsers) {
        std::string displayEmail = email;
        if (displayEmail.length() > 58) {
            displayEmail = displayEmail.substr(0, 55) + "...";
        }
        std::cout << "║ " << std::left << std::setw(60) << displayEmail << " ║" << std::endl;
    }

    std::cout << "╚══════════════════════════════════════════════════════════════╝\n" << std::endl;
}

} // namespace infrastructure::cli
