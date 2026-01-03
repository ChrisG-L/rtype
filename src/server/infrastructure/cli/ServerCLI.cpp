/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** ServerCLI implementation with TUI support
*/

#include "infrastructure/cli/ServerCLI.hpp"
#include "infrastructure/adapters/in/network/UDPServer.hpp"
#include "infrastructure/logging/Logger.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace infrastructure::cli {

ServerCLI::ServerCLI(std::shared_ptr<SessionManager> sessionManager,
                     UDPServer& udpServer,
                     std::shared_ptr<tui::LogBuffer> logBuffer)
    : _sessionManager(std::move(sessionManager))
    , _udpServer(udpServer)
    , _logBuffer(std::move(logBuffer))
{
    // Create TerminalUI if we have a log buffer
    if (_logBuffer) {
        _terminalUI = std::make_unique<tui::TerminalUI>(_logBuffer);
    }

    // Register commands
    _commands["help"] = [this](const std::string&) { printHelp(); };
    _commands["status"] = [this](const std::string&) { printStatus(); };
    _commands["sessions"] = [this](const std::string&) { listSessions(); };
    _commands["kick"] = [this](const std::string& args) { kickPlayer(args); };
    _commands["ban"] = [this](const std::string& args) { banUser(args); };
    _commands["unban"] = [this](const std::string& args) { unbanUser(args); };
    _commands["bans"] = [this](const std::string&) { listBans(); };
    _commands["logs"] = [this](const std::string& args) { toggleLogs(args); };
    _commands["zoom"] = [this](const std::string&) { enterZoomMode(); };
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

    // Start TUI if available
    if (_terminalUI) {
        _terminalUI->start();
    }

    _cliThread = std::jthread([this](std::stop_token) {
        runLoop();
    });
}

void ServerCLI::stop() {
    _running = false;

    // Stop TUI
    if (_terminalUI) {
        _terminalUI->stop();
    }

    output("[CLI] Stopping...");
}

void ServerCLI::join() {
    if (_cliThread.joinable()) {
        _cliThread.join();
    }
}

void ServerCLI::output(const std::string& text) {
    if (_terminalUI && _terminalUI->isRunning()) {
        _terminalUI->printToCommandPane(text);
    } else {
        std::cout << text << std::endl;
    }
}

void ServerCLI::runLoop() {
    output("");
    output("╔══════════════════════════════════════════════════╗");
    output("║   R-Type Server CLI - Type 'help' for commands   ║");
    output("╚══════════════════════════════════════════════════╝");
    output("");

    while (_running) {
        std::string command;

        if (_terminalUI && _terminalUI->isRunning()) {
            // TUI mode: get command from TerminalUI
            command = _terminalUI->processInputAndGetCommand();
            if (command.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
        } else {
            // Fallback: standard input (shouldn't happen normally)
            std::cout << "rtype> " << std::flush;
            if (!std::getline(std::cin, command)) {
                break;
            }
        }

        // Skip empty commands
        if (command.empty()) {
            continue;
        }

        executeCommand(command);
    }
}

void ServerCLI::executeCommand(const std::string& command) {
    auto args = parseArgs(command);
    if (args.empty()) {
        return;
    }

    std::string cmd = args[0];
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
    auto it = _commands.find(cmd);
    if (it != _commands.end()) {
        try {
            it->second(argStr);
        } catch (const std::exception& e) {
            output("[CLI] Error: " + std::string(e.what()));
        }
    } else {
        output("[CLI] Unknown command: " + cmd + ". Type 'help' for available commands.");
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
    output("");
    output("╔══════════════════════════════════════════════════════════════╗");
    output("║                      AVAILABLE COMMANDS                      ║");
    output("╠══════════════════════════════════════════════════════════════╣");
    output("║ help                  - Show this help message               ║");
    output("║ status                - Show server status                   ║");
    output("║ sessions              - List all active sessions             ║");
    output("║ kick <player_id>      - Kick a player by their in-game ID    ║");
    output("║ ban <email>           - Ban a user permanently               ║");
    output("║ unban <email>         - Unban a user                         ║");
    output("║ bans                  - List all banned users                ║");
    output("║ logs <on|off>         - Enable/disable server logs           ║");
    output("║ zoom                  - Full-screen log view (ESC to exit)   ║");
    output("║ quit/exit             - Stop the CLI (use Ctrl+C to stop     ║");
    output("║                         the server)                          ║");
    output("╠══════════════════════════════════════════════════════════════╣");
    output("║                      KEYBOARD SHORTCUTS                      ║");
    output("╠══════════════════════════════════════════════════════════════╣");
    output("║ 1                     - Filter: show all logs                ║");
    output("║ 2                     - Filter: info and above               ║");
    output("║ 3                     - Filter: warn and above               ║");
    output("║ 4                     - Filter: error only                   ║");
    output("║ Up/Down               - Scroll logs (1 line)                 ║");
    output("║ Page Up/Down          - Scroll logs (1 page)                 ║");
    output("╚══════════════════════════════════════════════════════════════╝");
    output("");
}

void ServerCLI::printStatus() {
    size_t sessionCount = _sessionManager->getSessionCount();
    size_t playerCount = _udpServer.getPlayerCount();

    std::ostringstream oss;
    output("");
    output("╔══════════════════════════════════════╗");
    output("║          SERVER STATUS               ║");
    output("╠══════════════════════════════════════╣");

    oss << "║ Active Sessions: " << std::setw(4) << sessionCount << "                ║";
    output(oss.str());
    oss.str("");

    oss << "║ Players in Game: " << std::setw(4) << playerCount << "                ║";
    output(oss.str());

    output("╚══════════════════════════════════════╝");
    output("");
}

void ServerCLI::listSessions() {
    auto sessions = _sessionManager->getAllSessions();

    if (sessions.empty()) {
        output("");
        output("[CLI] No active sessions.");
        output("");
        return;
    }

    output("");
    output("╔══════════════════════════════════════════════════════════════════════════════════╗");
    output("║                                ACTIVE SESSIONS                                    ║");
    output("╠══════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(25) << "Email"
           << std::setw(15) << "Display Name"
           << std::setw(10) << "Status"
           << std::setw(10) << "Player ID"
           << std::setw(22) << "Endpoint" << " ║";
    output(header.str());
    output("╠══════════════════════════════════════════════════════════════════════════════════╣");

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

        std::ostringstream row;
        row << "║ " << std::left << std::setw(25) << email
            << std::setw(15) << displayName
            << std::setw(10) << statusStr
            << std::setw(10) << playerIdStr
            << std::setw(22) << endpointStr << " ║";
        output(row.str());
    }

    output("╚══════════════════════════════════════════════════════════════════════════════════╝");
    output("");
}

void ServerCLI::kickPlayer(const std::string& args) {
    if (args.empty()) {
        output("[CLI] Usage: kick <player_id>");
        return;
    }

    try {
        int playerId = std::stoi(args);
        if (playerId < 0 || playerId > 255) {
            output("[CLI] Invalid player ID. Must be 0-255.");
            return;
        }
        _udpServer.kickPlayer(static_cast<uint8_t>(playerId));
        output("[CLI] Kicked player " + std::to_string(playerId));
    } catch (const std::exception&) {
        output("[CLI] Invalid player ID: " + args);
    }
}

void ServerCLI::toggleLogs(const std::string& args) {
    if (args.empty()) {
        // Toggle current state
        bool currentState = server::logging::Logger::isEnabled();
        server::logging::Logger::setEnabled(!currentState);
        output(std::string("[CLI] Logs ") + (!currentState ? "enabled" : "disabled"));
        return;
    }

    if (args == "on" || args == "1" || args == "true") {
        server::logging::Logger::setEnabled(true);
        output("[CLI] Logs enabled");
    } else if (args == "off" || args == "0" || args == "false") {
        server::logging::Logger::setEnabled(false);
        output("[CLI] Logs disabled");
    } else {
        output("[CLI] Usage: logs <on|off>");
    }
}

void ServerCLI::banUser(const std::string& args) {
    if (args.empty()) {
        output("[CLI] Usage: ban <email>");
        return;
    }

    if (_sessionManager->isBanned(args)) {
        output("[CLI] User '" + args + "' is already banned.");
        return;
    }

    _sessionManager->banUser(args);
    output("[CLI] User '" + args + "' has been banned.");
}

void ServerCLI::unbanUser(const std::string& args) {
    if (args.empty()) {
        output("[CLI] Usage: unban <email>");
        return;
    }

    if (!_sessionManager->isBanned(args)) {
        output("[CLI] User '" + args + "' is not banned.");
        return;
    }

    _sessionManager->unbanUser(args);
    output("[CLI] User '" + args + "' has been unbanned.");
}

void ServerCLI::listBans() {
    auto bannedUsers = _sessionManager->getBannedUsers();

    if (bannedUsers.empty()) {
        output("");
        output("[CLI] No banned users.");
        output("");
        return;
    }

    output("");
    output("╔══════════════════════════════════════════════════════════════╗");
    output("║                       BANNED USERS                            ║");
    output("╠══════════════════════════════════════════════════════════════╣");

    for (const auto& email : bannedUsers) {
        std::string displayEmail = email;
        if (displayEmail.length() > 58) {
            displayEmail = displayEmail.substr(0, 55) + "...";
        }
        std::ostringstream row;
        row << "║ " << std::left << std::setw(60) << displayEmail << " ║";
        output(row.str());
    }

    output("╚══════════════════════════════════════════════════════════════╝");
    output("");
}

void ServerCLI::enterZoomMode() {
    if (_terminalUI) {
        _terminalUI->setMode(tui::TerminalUI::Mode::ZoomLogs);
        output("[CLI] Entered zoom mode. Press ESC to exit.");
    } else {
        output("[CLI] Zoom mode not available (TUI not initialized).");
    }
}

} // namespace infrastructure::cli
