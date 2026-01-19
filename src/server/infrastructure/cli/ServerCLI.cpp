/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** ServerCLI implementation with TUI support
*/

#include "infrastructure/cli/ServerCLI.hpp"
#include "infrastructure/adapters/in/network/UDPServer.hpp"
#include "infrastructure/logging/Logger.hpp"
#include "infrastructure/tui/Utf8Utils.hpp"
#include "infrastructure/network/NetworkStats.hpp"
#include "domain/value_objects/user/Email.hpp"
#include "domain/value_objects/user/Username.hpp"
#include "domain/value_objects/user/Password.hpp"
#include "domain/value_objects/user/utils/PasswordUtils.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <set>
#include <algorithm>
#include <mutex>
#include <chrono>

#ifdef _WIN32
// WIN32_LEAN_AND_MEAN prevents windows.h from including winsock.h
// which would conflict with winsock2.h already included by Boost.Asio
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
// Cross-platform localtime: Windows uses localtime_s with reversed args
inline std::tm* localtime_r(const std::time_t* timer, std::tm* buf) {
    return localtime_s(buf, timer) == 0 ? buf : nullptr;
}
#endif

namespace infrastructure::cli {

ServerCLI::ServerCLI(std::shared_ptr<SessionManager> sessionManager,
                     UDPServer& udpServer,
                     std::shared_ptr<tui::LogBuffer> logBuffer,
                     std::shared_ptr<IUserRepository> userRepository,
                     std::shared_ptr<RoomManager> roomManager,
                     std::shared_ptr<IPrivateMessageRepository> pmRepository)
    : _sessionManager(std::move(sessionManager))
    , _udpServer(udpServer)
    , _logBuffer(std::move(logBuffer))
    , _userRepository(std::move(userRepository))
    , _roomManager(std::move(roomManager))
    , _pmRepository(std::move(pmRepository))
{
    // Create TerminalUI if we have a log buffer
    if (_logBuffer) {
        _terminalUI = std::make_unique<tui::TerminalUI>(_logBuffer);
    }

    // Register commands
    _commands["help"] = [this](const std::string&) { printHelp(); };
    _commands["status"] = [this](const std::string&) { printStatus(); };
    _commands["sessions"] = [this](const std::string&) { listSessions(); };
    _commands["rooms"] = [this](const std::string&) { listRooms(); };
    _commands["room"] = [this](const std::string& args) { showRoom(args); };
    _commands["closeroom"] = [this](const std::string& args) { closeRoom(args); };
    _commands["kickfromroom"] = [this](const std::string& args) { kickFromRoom(args); };
    _commands["kick"] = [this](const std::string& args) { kickPlayer(args); };
    _commands["ban"] = [this](const std::string& args) { banUser(args); };
    _commands["unban"] = [this](const std::string& args) { unbanUser(args); };
    _commands["bans"] = [this](const std::string&) { listBans(); };
    _commands["users"] = [this](const std::string&) { listUsers(); };
    _commands["user"] = [this](const std::string& args) { showUser(args); };
    _commands["logs"] = [this](const std::string& args) { toggleLogs(args); };
    _commands["debug"] = [this](const std::string& args) { toggleDebug(args); };
    _commands["zoom"] = [this](const std::string&) { enterZoomMode(); };
    _commands["interact"] = [this](const std::string& args) { enterInteractMode(args); };
    _commands["net"] = [this](const std::string& args) { cmdNet(args); };
    _commands["pmstats"] = [this](const std::string& args) { pmStats(args); };
    _commands["pmuser"] = [this](const std::string& args) { pmUser(args); };
    _commands["pmconv"] = [this](const std::string& args) { pmConversation(args); };
    _commands["pmsearch"] = [this](const std::string& args) { pmSearch(args); };
    _commands["pmrecent"] = [this](const std::string& args) { pmRecent(args); };
    _commands["quit"] = [this](const std::string&) { stop(); };
    _commands["exit"] = [this](const std::string&) { stop(); };

    // Set up interact action callback
    if (_terminalUI) {
        _terminalUI->setInteractActionCallback(
            [this](tui::InteractAction action, const tui::SelectableElement& element) {
                handleInteractAction(action, element);
            });
        _terminalUI->setEditConfirmCallback(
            [this](const tui::SelectableElement& element, const std::string& newValue) {
                handleEditConfirm(element, newValue);
            });
    }
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
    if (!_running.exchange(false)) {
        return;  // Already stopped
    }

    // Stop TUI
    if (_terminalUI) {
        _terminalUI->stop();
    }

    output("[CLI] Stopping...");

    // Notify GameBootstrap to stop io_ctx (this allows exit/quit to actually stop the server)
    if (_shutdownCallback) {
        _shutdownCallback();
    }
}

void ServerCLI::setShutdownCallback(ShutdownCallback callback) {
    _shutdownCallback = std::move(callback);
}

void ServerCLI::join() {
    if (_cliThread.joinable()) {
        _cliThread.join();
    }
}

void ServerCLI::output(const std::string& text) {
    // If output callback is set (remote admin), use it
    if (_outputCallback) {
        _outputCallback(text);
        return;
    }
    // Otherwise use TUI or stdout
    if (_terminalUI && _terminalUI->isRunning()) {
        _terminalUI->printToCommandPane(text);
    } else {
        std::cout << text << std::endl;
    }
}

std::vector<std::string> ServerCLI::executeCommandWithOutput(const std::string& command) {
    std::vector<std::string> outputLines;

    // Set output callback to capture output
    {
        std::lock_guard<std::mutex> lock(_outputMutex);
        _outputCallback = [&outputLines](const std::string& text) {
            outputLines.push_back(text);
        };
    }

    // Execute the command
    executeCommand(command);

    // Clear the callback
    {
        std::lock_guard<std::mutex> lock(_outputMutex);
        _outputCallback = nullptr;
    }

    return outputLines;
}

std::vector<std::string> ServerCLI::getAvailableCommands() const {
    std::vector<std::string> cmds;
    cmds.reserve(_commands.size());
    for (const auto& [name, _] : _commands) {
        cmds.push_back(name);
    }
    std::sort(cmds.begin(), cmds.end());
    return cmds;
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
            // TUI stopped or not available - check if we're shutting down
            if (!_running) {
                break;
            }
            // Fallback: standard input - sleep briefly and continue to avoid blocking
            // Note: std::getline blocks, so we just exit the loop when TUI stops
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
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
    output("║ help                 - Show this help message                ║");
    output("║ status               - Show server status                    ║");
    output("║ sessions             - List all active sessions              ║");
    output("║ rooms                - List all active rooms                 ║");
    output("║ room <code>          - Show room details + chat history      ║");
    output("║ closeroom <code>     - Force close a room (kicks all)        ║");
    output("║ users                - List all registered users (DB)        ║");
    output("║ user <email>         - Show user details                     ║");
    output("║ kick <email> [reason]- Kick player (from room and/or game)   ║");
    output("║ ban <email>          - Ban a user permanently                ║");
    output("║ unban <email>        - Unban a user                          ║");
    output("║ bans                 - List all banned users                 ║");
    output("╠══════════════════════════════════════════════════════════════╣");
    output("║                    PRIVATE MESSAGES (ADMIN)                  ║");
    output("╠══════════════════════════════════════════════════════════════╣");
    output("║ pmstats              - Show PM statistics                    ║");
    output("║ pmuser <email>       - Show all messages for a user          ║");
    output("║ pmconv <e1> <e2>     - Show conversation between 2 users     ║");
    output("║ pmsearch <term>      - Search messages by content            ║");
    output("║ pmrecent [limit]     - Show recent messages (default 50)     ║");
    output("╠══════════════════════════════════════════════════════════════╣");
    output("║ logs <on|off>        - Enable/disable all server logs        ║");
    output("║ debug <on|off>       - Enable/disable debug logs             ║");
    output("║ zoom                 - Full-screen log view (ESC to exit)    ║");
    output("║ net                  - Real-time network monitor (tree view) ║");
    output("║ interact [cmd]       - Navigate output (sessions/bans/users/ ║");
    output("║                        rooms/room/user)                      ║");
    output("║ quit/exit            - Stop the server                       ║");
    output("╠══════════════════════════════════════════════════════════════╣");
    output("║                      KEYBOARD SHORTCUTS                      ║");
    output("╠══════════════════════════════════════════════════════════════╣");
    output("║ 1                    - Filter: show all logs                 ║");
    output("║ 2                    - Filter: info and above                ║");
    output("║ 3                    - Filter: warn and above                ║");
    output("║ 4                    - Filter: error only                    ║");
    output("║ Up/Down              - Scroll logs (1 line)                  ║");
    output("║ Page Up/Down         - Scroll logs (1 page)                  ║");
    output("╚══════════════════════════════════════════════════════════════╝");
    output("");
}

void ServerCLI::printStatus() {
    size_t sessionCount = _sessionManager->getSessionCount();
    size_t playerCount = _udpServer.getPlayerCount();
    size_t bannedCount = _sessionManager->getBannedUsers().size();
    size_t usersInDb = _userRepository ? _userRepository->findAll().size() : 0;
    size_t roomCount = _roomManager ? _roomManager->getRoomCount() : 0;
    bool logsEnabled = server::logging::Logger::isEnabled();
    bool debugEnabled = server::logging::Logger::isDebugEnabled();

    std::ostringstream oss;
    output("");
    output("╔═════════════════════════════════════╗");
    output("║            SERVER STATUS            ║");
    output("╠═════════════════════════════════════╣");

    oss << "║ Active Sessions: " << std::setw(4) << sessionCount << "               ║";
    output(oss.str());
    oss.str("");

    oss << "║ Players in Game: " << std::setw(4) << playerCount << "               ║";
    output(oss.str());
    oss.str("");

    oss << "║ Active Rooms:    " << std::setw(4) << roomCount << "               ║";
    output(oss.str());
    oss.str("");

    oss << "║ Banned Users:    " << std::setw(4) << bannedCount << "               ║";
    output(oss.str());
    oss.str("");

    oss << "║ Users in DB:     " << std::setw(4) << usersInDb << "               ║";
    output(oss.str());

    output("╠═════════════════════════════════════╣");

    oss.str("");
    oss << "║ Logs:   " << std::left << std::setw(10) << (logsEnabled ? "ON" : "OFF")
        << "  Debug: " << std::setw(6) << (debugEnabled ? "ON" : "OFF") << "   ║";
    output(oss.str());

    output("╚═════════════════════════════════════╝");
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
    output("╔═══════════════════════════════════════════════════════════════════════════════════════════════╗");
    output("║                                        ACTIVE SESSIONS                                        ║");
    output("╠═══════════════════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(25) << "Email"
           << std::setw(15) << "Display Name"
           << std::setw(10) << "Status"
           << std::setw(9) << "Room"
           << std::setw(10) << "Player ID"
           << std::setw(22) << "Endpoint" << "   ║";
    output(header.str());
    output("╠═══════════════════════════════════════════════════════════════════════════════════════════════╣");

    for (const auto& session : sessions) {
        std::string statusStr;
        switch (session.status) {
            case session::Session::Status::Pending: statusStr = "Pending"; break;
            case session::Session::Status::Active: statusStr = "Active"; break;
            case session::Session::Status::Expired: statusStr = "Expired"; break;
        }

        std::string playerIdStr = session.playerId ? std::to_string(*session.playerId) : "-";
        std::string endpointStr = session.udpEndpoint.empty() ? "-" : session.udpEndpoint;

        // Get room code if player is in a room
        std::string roomCode = "-";
        if (_roomManager && _roomManager->isPlayerInRoom(session.email)) {
            auto* room = _roomManager->getRoomByPlayerEmail(session.email);
            if (room) {
                roomCode = room->getCode();
            }
        }

        // Truncate long strings (UTF-8 aware)
        std::string email = tui::utf8::truncateWithEllipsis(session.email, 24);
        std::string displayName = tui::utf8::truncateWithEllipsis(session.displayName, 14);
        endpointStr = tui::utf8::truncateWithEllipsis(endpointStr, 21);

        std::ostringstream row;
        row << "║ " << std::left << std::setw(25) << email
            << std::setw(15) << displayName
            << std::setw(10) << statusStr
            << std::setw(9) << roomCode
            << std::setw(10) << playerIdStr
            << std::setw(22) << endpointStr << "   ║";
        output(row.str());
    }

    output("╚═══════════════════════════════════════════════════════════════════════════════════════════════╝");
    output("");

    // Build and store interactive output for interact mode
    _lastInteractiveOutput = buildSessionsInteractiveOutput();
    _lastCommand = "sessions";
    if (_terminalUI) {
        _terminalUI->setInteractiveOutput(_lastInteractiveOutput);
    }
}

void ServerCLI::kickPlayer(const std::string& args) {
    if (args.empty()) {
        output("[CLI] Usage: kick <email> [reason]");
        return;
    }

    // Parse email and optional reason
    auto parts = parseArgs(args);
    if (parts.empty()) {
        output("[CLI] Usage: kick <email> [reason]");
        return;
    }

    std::string email = parts[0];

    // Build reason from remaining args
    std::string reason = "Kicked by administrator";
    if (parts.size() > 1) {
        reason.clear();
        for (size_t i = 1; i < parts.size(); ++i) {
            if (!reason.empty()) reason += " ";
            reason += parts[i];
        }
    }

    // Step 1: Verify the player has an active session
    auto sessionOpt = _sessionManager->getSessionByEmail(email);
    if (!sessionOpt) {
        output("[CLI] Player not found: " + email);
        return;
    }

    // Step 2: Notify the player via TCP (sends PlayerKickedNotification)
    bool notified = _sessionManager->kickPlayerByEmail(email, reason);
    if (notified) {
        output("[CLI] Player notified via TCP");
    }

    // Step 3: Remove from room if in one
    if (_roomManager && _roomManager->isPlayerInRoom(email)) {
        _roomManager->leaveRoom(email);
        output("[CLI] Player removed from room");
    }

    // Step 4: If player is in game (has playerId), kick from UDP too
    if (sessionOpt->playerId.has_value()) {
        uint8_t playerId = *sessionOpt->playerId;
        _udpServer.kickPlayer(playerId);
        output("[CLI] Player removed from game (ID: " + std::to_string(playerId) + ")");
    }

    output("[CLI] Kicked " + email + " (reason: " + reason + ")");
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

void ServerCLI::toggleDebug(const std::string& args) {
    if (args.empty()) {
        // Toggle current state
        bool currentState = server::logging::Logger::isDebugEnabled();
        server::logging::Logger::setDebugEnabled(!currentState);
        output(std::string("[CLI] Debug logs ") + (!currentState ? "enabled" : "disabled"));
        return;
    }

    if (args == "on" || args == "1" || args == "true") {
        server::logging::Logger::setDebugEnabled(true);
        output("[CLI] Debug logs enabled");
    } else if (args == "off" || args == "0" || args == "false") {
        server::logging::Logger::setDebugEnabled(false);
        output("[CLI] Debug logs disabled");
    } else {
        output("[CLI] Usage: debug <on|off>");
    }
}

void ServerCLI::banUser(const std::string& args) {
    if (args.empty()) {
        output("[CLI] Usage: ban <email>");
        return;
    }

    std::string email = args;

    if (_sessionManager->isBanned(email)) {
        output("[CLI] User '" + email + "' is already banned.");
        return;
    }

    // Step 1: If player has an active session, kick them first
    auto sessionOpt = _sessionManager->getSessionByEmail(email);
    if (sessionOpt) {
        // Notify the player via TCP (sends PlayerKickedNotification with ban reason)
        bool notified = _sessionManager->kickPlayerByEmail(email, "You have been banned");
        if (notified) {
            output("[CLI] Player notified via TCP");
        }

        // Remove from room if in one
        if (_roomManager && _roomManager->isPlayerInRoom(email)) {
            _roomManager->leaveRoom(email);
            output("[CLI] Player removed from room");
        }

        // If player is in game (has playerId), kick from UDP too
        if (sessionOpt->playerId.has_value()) {
            uint8_t playerId = *sessionOpt->playerId;
            _udpServer.kickPlayer(playerId);
            output("[CLI] Player removed from game (ID: " + std::to_string(playerId) + ")");
        }
    }

    // Step 2: Ban the user (this also removes any remaining session)
    _sessionManager->banUser(email);
    output("[CLI] User '" + email + "' has been banned.");
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
    output("╔═════════════════════════════════════════════════════════════════════════════════╗");
    output("║                                  BANNED USERS                                   ║");
    output("╠═════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(40) << "Email"
           << std::setw(38) << "Display Name" << "  ║";
    output(header.str());
    output("╠═════════════════════════════════════════════════════════════════════════════════╣");

    for (const auto& banned : bannedUsers) {
        // Truncate fields (UTF-8 aware)
        std::string emailTrunc = tui::utf8::truncateWithEllipsis(banned.email, 39);
        std::string displayNameTrunc = banned.displayName.empty()
            ? "-"
            : tui::utf8::truncateWithEllipsis(banned.displayName, 37);

        std::ostringstream row;
        row << "║ " << std::left << std::setw(40) << emailTrunc
            << std::setw(38) << displayNameTrunc << "  ║";
        output(row.str());
    }

    output("╚═════════════════════════════════════════════════════════════════════════════════╝");
    output("");

    // Build and store interactive output for interact mode
    _lastInteractiveOutput = buildBansInteractiveOutput();
    _lastCommand = "bans";
    if (_terminalUI) {
        _terminalUI->setInteractiveOutput(_lastInteractiveOutput);
    }
}

void ServerCLI::listUsers() {
    if (!_userRepository) {
        output("[CLI] User repository not available.");
        return;
    }

    auto users = _userRepository->findAll();

    if (users.empty()) {
        output("");
        output("[CLI] No registered users in database.");
        output("");
        return;
    }

    output("");
    output("╔═════════════════════════════════════════════════════════════════════════════════╗");
    output("║                                REGISTERED USERS                                 ║");
    output("╠═════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(40) << "Email"
           << std::setw(25) << "Username"
           << std::setw(14) << "Status" << " ║";
    output(header.str());
    output("╠═════════════════════════════════════════════════════════════════════════════════╣");

    for (const auto& user : users) {
        std::string email = user.getEmail().value();
        std::string username = user.getUsername().value();

        // Check status
        std::string statusStr;
        if (_sessionManager->isBanned(email)) {
            statusStr = "Banned";
        } else if (_sessionManager->hasActiveSession(email)) {
            statusStr = "Online";
        } else {
            statusStr = "Offline";
        }

        // Truncate fields (UTF-8 aware)
        std::string emailTrunc = tui::utf8::truncateWithEllipsis(email, 39);
        std::string usernameTrunc = tui::utf8::truncateWithEllipsis(username, 24);

        std::ostringstream row;
        row << "║ " << std::left << std::setw(40) << emailTrunc
            << std::setw(25) << usernameTrunc
            << std::setw(14) << statusStr << " ║";
        output(row.str());
    }

    output("╚═════════════════════════════════════════════════════════════════════════════════╝");
    output("");
    output("[CLI] Total: " + std::to_string(users.size()) + " user(s)");
    output("");

    // Build and store interactive output for interact mode
    _lastInteractiveOutput = buildUsersInteractiveOutput();
    _lastCommand = "users";
    if (_terminalUI) {
        _terminalUI->setInteractiveOutput(_lastInteractiveOutput);
    }
}

void ServerCLI::showUser(const std::string& args) {
    if (args.empty()) {
        output("[CLI] Usage: user <email>");
        return;
    }

    if (!_userRepository) {
        output("[CLI] User repository not available.");
        return;
    }

    auto userOpt = _userRepository->findByEmail(args);
    if (!userOpt) {
        output("[CLI] User not found: " + args);
        return;
    }

    const auto& user = *userOpt;
    std::string email = user.getEmail().value();
    std::string username = user.getUsername().value();
    std::string oderId = user.getId().value();
    std::string passwordHash = user.getPasswordHash().value();

    // Get status from session manager
    std::string statusStr;
    if (_sessionManager->isBanned(email)) {
        statusStr = "Banned";
    } else if (_sessionManager->hasActiveSession(email)) {
        statusStr = "Online";
    } else {
        statusStr = "Offline";
    }

    // Format timestamps
    auto formatTime = [](const std::chrono::system_clock::time_point& tp) -> std::string {
        auto time = std::chrono::system_clock::to_time_t(tp);
        std::tm tm = *std::localtime(&time);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    };

    std::string createdAt = formatTime(user.getCreatedAt());
    std::string lastLogin = formatTime(user.getLastLogin());

    output("");
    output("╔═════════════════════════════════════════════════════════════════╗");
    output("║                         USER DETAILS                            ║");
    output("╠═════════════════════════════════════════════════════════════════╣");

    std::ostringstream oss;
    oss << "║ Email:      " << std::left << std::setw(52) << email << "║";
    output(oss.str());
    oss.str("");

    oss << "║ Username:   " << std::left << std::setw(52) << username << "║";
    output(oss.str());
    oss.str("");

    // Truncate password hash for display (it can be long)
    std::string hashTrunc = tui::utf8::truncateWithEllipsis(passwordHash, 51);
    oss << "║ Password:   " << std::left << std::setw(52) << hashTrunc << "║";
    output(oss.str());
    oss.str("");

    oss << "║ User ID:    " << std::left << std::setw(52) << oderId << "║";
    output(oss.str());
    oss.str("");

    oss << "║ Status:     " << std::left << std::setw(52) << statusStr << "║";
    output(oss.str());
    oss.str("");

    output("╠═════════════════════════════════════════════════════════════════╣");

    oss << "║ Created:    " << std::left << std::setw(52) << createdAt << "║";
    output(oss.str());
    oss.str("");

    oss << "║ Last Login: " << std::left << std::setw(52) << lastLogin << "║";
    output(oss.str());

    output("╚═════════════════════════════════════════════════════════════════╝");
    output("");

    // Store interactive output for interact mode
    _lastInteractiveOutput = buildUserDetailsInteractiveOutput(email);
    _lastCommand = "user";
    if (_terminalUI) {
        _terminalUI->setInteractiveOutput(_lastInteractiveOutput);
    }
}

void ServerCLI::listRooms() {
    if (!_roomManager) {
        output("[CLI] Room manager not available.");
        return;
    }

    auto rooms = _roomManager->getAllRooms();

    if (rooms.empty()) {
        output("");
        output("[CLI] No active rooms.");
        output("");
        return;
    }

    output("");
    output("╔══════════════════════════════════════════════════════════════════════════════════╗");
    output("║                                   ACTIVE ROOMS                                   ║");
    output("╠══════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(8) << "Code"
           << std::setw(25) << "Name"
           << std::setw(10) << "Players"
           << std::setw(12) << "State"
           << std::setw(10) << "Private"
           << std::setw(15) << "Host" << " ║";
    output(header.str());
    output("╠══════════════════════════════════════════════════════════════════════════════════╣");

    for (const auto* room : rooms) {
        std::string stateStr;
        switch (room->getState()) {
            case domain::entities::Room::State::Waiting: stateStr = "Waiting"; break;
            case domain::entities::Room::State::Starting: stateStr = "Starting"; break;
            case domain::entities::Room::State::InGame: stateStr = "InGame"; break;
            case domain::entities::Room::State::Closed: stateStr = "Closed"; break;
        }

        std::string playersStr = std::to_string(room->getPlayerCount()) + "/" +
                                 std::to_string(room->getMaxPlayers());

        // Truncate fields (UTF-8 aware)
        std::string codeTrunc = tui::utf8::truncateWithEllipsis(room->getCode(), 7);
        std::string nameTrunc = tui::utf8::truncateWithEllipsis(room->getName(), 24);
        std::string hostTrunc = tui::utf8::truncateWithEllipsis(room->getHostEmail(), 14);

        std::ostringstream row;
        row << "║ " << std::left << std::setw(8) << codeTrunc
            << std::setw(25) << nameTrunc
            << std::setw(10) << playersStr
            << std::setw(12) << stateStr
            << std::setw(10) << (room->isPrivate() ? "Yes" : "No")
            << std::setw(15) << hostTrunc << " ║";
        output(row.str());
    }

    output("╚══════════════════════════════════════════════════════════════════════════════════╝");
    output("");
    output("[CLI] Total: " + std::to_string(rooms.size()) + " room(s)");
    output("");

    // Build and store interactive output for interact mode
    _lastInteractiveOutput = buildRoomsInteractiveOutput();
    _lastCommand = "rooms";
    if (_terminalUI) {
        _terminalUI->setInteractiveOutput(_lastInteractiveOutput);
    }
}

void ServerCLI::showRoom(const std::string& args) {
    if (args.empty()) {
        output("[CLI] Usage: room <code>");
        return;
    }

    if (!_roomManager) {
        output("[CLI] Room manager not available.");
        return;
    }

    auto* room = _roomManager->getRoomByCode(args);
    if (!room) {
        output("[CLI] Room not found: " + args);
        return;
    }

    std::string stateStr;
    switch (room->getState()) {
        case domain::entities::Room::State::Waiting: stateStr = "Waiting"; break;
        case domain::entities::Room::State::Starting: stateStr = "Starting"; break;
        case domain::entities::Room::State::InGame: stateStr = "InGame"; break;
        case domain::entities::Room::State::Closed: stateStr = "Closed"; break;
    }

    output("");
    output("╔═════════════════════════════════════════════════════════════════╗");
    output("║                         ROOM DETAILS                            ║");
    output("╠═════════════════════════════════════════════════════════════════╣");

    std::ostringstream oss;
    oss << "║ Code:       " << std::left << std::setw(52) << room->getCode() << "║";
    output(oss.str());
    oss.str("");

    oss << "║ Name:       " << std::left << std::setw(52) << room->getName() << "║";
    output(oss.str());
    oss.str("");

    oss << "║ Host:       " << std::left << std::setw(52) << room->getHostEmail() << "║";
    output(oss.str());
    oss.str("");

    std::string playersStr = std::to_string(room->getPlayerCount()) + "/" +
                             std::to_string(room->getMaxPlayers());
    oss << "║ Players:    " << std::left << std::setw(52) << playersStr << "║";
    output(oss.str());
    oss.str("");

    oss << "║ State:      " << std::left << std::setw(52) << stateStr << "║";
    output(oss.str());
    oss.str("");

    oss << "║ Private:    " << std::left << std::setw(52) << (room->isPrivate() ? "Yes" : "No") << "║";
    output(oss.str());

    output("╠═════════════════════════════════════════════════════════════════╣");
    output("║                             PLAYERS                             ║");
    output("╠═════════════════════════════════════════════════════════════════╣");

    std::ostringstream playerHeader;
    playerHeader << "║ " << std::left << std::setw(6) << "Slot"
                 << std::setw(20) << "Display Name"
                 << std::setw(10) << "Ready"
                 << std::setw(10) << "Host"
                 << std::setw(18) << "Email" << "║";
    output(playerHeader.str());
    output("╠═════════════════════════════════════════════════════════════════╣");

    const auto& slots = room->getSlots();
    for (size_t i = 0; i < domain::entities::Room::MAX_SLOTS; ++i) {
        if (slots[i].occupied) {
            std::string nameTrunc = tui::utf8::truncateWithEllipsis(slots[i].displayName, 19);
            std::string emailTrunc = tui::utf8::truncateWithEllipsis(slots[i].email, 17);

            std::ostringstream playerRow;
            playerRow << "║ " << std::left << std::setw(6) << i
                      << std::setw(20) << nameTrunc
                      << std::setw(10) << (slots[i].isReady ? "Yes" : "No")
                      << std::setw(10) << (slots[i].isHost ? "Yes" : "No")
                      << std::setw(18) << emailTrunc << "║";
            output(playerRow.str());
        }
    }

    // Chat history section
    const auto& chatHistory = room->getChatHistory();
    if (!chatHistory.empty()) {
        output("╠═════════════════════════════════════════════════════════════════╣");
        output("║                        CHAT HISTORY                             ║");
        output("╠═════════════════════════════════════════════════════════════════╣");

        // Show last 10 messages
        size_t startIdx = chatHistory.size() > 10 ? chatHistory.size() - 10 : 0;
        for (size_t i = startIdx; i < chatHistory.size(); ++i) {
            const auto& msg = chatHistory[i];
            std::string senderTrunc = tui::utf8::truncateWithEllipsis(msg.displayName, 12);
            std::string msgTrunc = tui::utf8::truncateWithEllipsis(msg.message, 48);

            std::ostringstream chatRow;
            chatRow << "║ " << std::left << std::setw(14) << ("[" + senderTrunc + "]")
                    << std::setw(50) << msgTrunc << " ║";
            output(chatRow.str());
        }

        if (startIdx > 0) {
            std::ostringstream moreRow;
            moreRow << "║ " << std::left << std::setw(62)
                    << ("... " + std::to_string(startIdx) + " more message(s)") << " ║";
            output(moreRow.str());
        }
    }

    output("╚═════════════════════════════════════════════════════════════════╝");
    output("");

    // Store interactive output for interact mode
    _lastInteractiveOutput = buildRoomDetailsInteractiveOutput(args);
    _lastCommand = "room";
    if (_terminalUI) {
        _terminalUI->setInteractiveOutput(_lastInteractiveOutput);
    }
}

void ServerCLI::closeRoom(const std::string& args) {
    if (args.empty()) {
        output("[CLI] Usage: closeroom <code>");
        return;
    }

    if (!_roomManager) {
        output("[CLI] Room manager not available.");
        return;
    }

    std::string code = args;

    // Check if room exists first
    auto* room = _roomManager->getRoomByCode(code);
    if (!room) {
        output("[CLI] Room not found: " + code);
        return;
    }

    std::string roomName = room->getName();
    size_t playerCount = _roomManager->forceCloseRoom(code);

    output("[CLI] Room '" + roomName + "' (" + code + ") closed.");
    output("[CLI] " + std::to_string(playerCount) + " player(s) were disconnected.");
}

void ServerCLI::kickFromRoom(const std::string& args) {
    auto params = parseArgs(args);
    if (params.size() < 2) {
        output("[CLI] Usage: kickfromroom <code> <email> [reason]");
        return;
    }

    if (!_roomManager) {
        output("[CLI] Room manager not available.");
        return;
    }

    std::string code = params[0];
    std::string email = params[1];
    std::string reason;
    if (params.size() > 2) {
        // Collect remaining args as reason
        for (size_t i = 2; i < params.size(); ++i) {
            if (!reason.empty()) reason += " ";
            reason += params[i];
        }
    }

    // Check if room exists
    auto* room = _roomManager->getRoomByCode(code);
    if (!room) {
        output("[CLI] Room not found: " + code);
        return;
    }

    // Check if it's the host
    if (room->isHost(email)) {
        output("[CLI] Cannot kick the host. Use 'closeroom " + code + "' to close the room instead.");
        return;
    }

    std::string result = _roomManager->adminKickFromRoom(code, email, reason);
    if (result.empty()) {
        output("[CLI] Player not found in room: " + email);
    } else {
        output("[CLI] Kicked " + email + " from room " + code);
    }
}

void ServerCLI::enterZoomMode() {
    if (_terminalUI) {
        _terminalUI->setMode(tui::TerminalUI::Mode::ZoomLogs);
        output("[CLI] Entered zoom mode. Press ESC to exit.");
    } else {
        output("[CLI] Zoom mode not available (TUI not initialized).");
    }
}

void ServerCLI::enterInteractMode(const std::string& args) {
    if (!_terminalUI) {
        output("[CLI] Interact mode not available (TUI not initialized).");
        return;
    }

    // If a command is specified, run it first to populate interactive output
    if (!args.empty()) {
        if (args == "sessions") {
            listSessions();
        } else if (args == "bans") {
            listBans();
        } else if (args == "users") {
            listUsers();
        } else if (args == "rooms") {
            listRooms();
        } else if (args.rfind("room ", 0) == 0) {
            // "room <code>" - show specific room details
            std::string roomCode = args.substr(5);
            if (roomCode.empty()) {
                output("[CLI] Usage: interact room <code>");
                return;
            }
            showRoom(roomCode);
        } else if (args.rfind("user ", 0) == 0) {
            // "user <email>" - show specific user details
            std::string userEmail = args.substr(5);
            if (userEmail.empty()) {
                output("[CLI] Usage: interact user <email>");
                return;
            }
            showUser(userEmail);
        } else {
            output("[CLI] Unknown interact target: " + args);
            output("[CLI] Valid targets: sessions, bans, users, rooms, room <code>, user <email>");
            return;
        }
    }

    if (_lastInteractiveOutput.lines.empty()) {
        output("[CLI] No output to interact with. Run 'sessions', 'bans', 'users' or 'rooms' first.");
        return;
    }

    if (!_lastInteractiveOutput.hasSelectables()) {
        output("[CLI] Last output has no selectable elements.");
        return;
    }

    _terminalUI->enterInteractMode();
    output("[CLI] Entering interact mode. Use ←→ to select, ↑↓ to scroll, ESC to exit.");
}

tui::InteractiveOutput ServerCLI::buildSessionsInteractiveOutput() {
    tui::InteractiveOutput output;
    output.sourceCommand = "sessions";

    auto sessions = _sessionManager->getAllSessions();
    if (sessions.empty()) {
        return output;
    }

    // Build lines (same format as listSessions)
    output.lines.push_back("");
    output.lines.push_back("╔═══════════════════════════════════════════════════════════════════════════════════════════════╗");
    output.lines.push_back("║                                        ACTIVE SESSIONS                                        ║");
    output.lines.push_back("╠═══════════════════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(25) << "Email"
           << std::setw(15) << "Display Name"
           << std::setw(10) << "Status"
           << std::setw(9) << "Room"
           << std::setw(10) << "Player ID"
           << std::setw(22) << "Endpoint" << "   ║";
    output.lines.push_back(header.str());
    output.lines.push_back("╠═══════════════════════════════════════════════════════════════════════════════════════════════╣");

    size_t lineIdx = output.lines.size();  // First data line

    for (const auto& session : sessions) {
        std::string statusStr;
        switch (session.status) {
            case session::Session::Status::Pending: statusStr = "Pending"; break;
            case session::Session::Status::Active: statusStr = "Active"; break;
            case session::Session::Status::Expired: statusStr = "Expired"; break;
        }

        std::string playerIdStr = session.playerId ? std::to_string(*session.playerId) : "-";
        std::string endpointStr = session.udpEndpoint.empty() ? "-" : session.udpEndpoint;

        // Get room code if player is in a room
        std::string roomCode = "-";
        if (_roomManager && _roomManager->isPlayerInRoom(session.email)) {
            auto* room = _roomManager->getRoomByPlayerEmail(session.email);
            if (room) {
                roomCode = room->getCode();
            }
        }

        // Truncate for display
        std::string emailTrunc = tui::utf8::truncateWithEllipsis(session.email, 24);
        std::string displayNameTrunc = tui::utf8::truncateWithEllipsis(session.displayName, 14);
        std::string endpointTrunc = tui::utf8::truncateWithEllipsis(endpointStr, 21);

        std::ostringstream row;
        row << "║ " << std::left << std::setw(25) << emailTrunc
            << std::setw(15) << displayNameTrunc
            << std::setw(10) << statusStr
            << std::setw(9) << roomCode
            << std::setw(10) << playerIdStr
            << std::setw(22) << endpointTrunc << "   ║";
        output.lines.push_back(row.str());

        // Column positions (after "║ ")
        size_t col = 2;  // Start after "║ "

        // Email element (25 columns)
        tui::SelectableElement emailElem;
        emailElem.lineIndex = lineIdx;
        emailElem.startCol = col;
        emailElem.endCol = col + 25;
        emailElem.value = session.email;
        emailElem.truncatedValue = emailTrunc;
        emailElem.type = tui::ElementType::Email;
        if (session.playerId) {
            emailElem.associatedPlayerId = *session.playerId;
        }
        output.elements.push_back(emailElem);
        col += 25;

        // Display Name element (15 columns)
        tui::SelectableElement nameElem;
        nameElem.lineIndex = lineIdx;
        nameElem.startCol = col;
        nameElem.endCol = col + 15;
        nameElem.value = session.displayName;
        nameElem.truncatedValue = displayNameTrunc;
        nameElem.type = tui::ElementType::DisplayName;
        nameElem.associatedEmail = session.email;
        if (session.playerId) {
            nameElem.associatedPlayerId = *session.playerId;
        }
        output.elements.push_back(nameElem);
        col += 15;

        // Status is not selectable, skip it
        col += 10;

        // Room code element (8 columns) - selectable if in a room
        if (roomCode != "-") {
            tui::SelectableElement roomElem;
            roomElem.lineIndex = lineIdx;
            roomElem.startCol = col;
            roomElem.endCol = col + 8;
            roomElem.value = roomCode;
            roomElem.truncatedValue = roomCode;
            roomElem.type = tui::ElementType::RoomCode;
            roomElem.associatedRoomCode = roomCode;
            roomElem.associatedEmail = session.email;
            output.elements.push_back(roomElem);
        }
        col += 9;

        // Player ID element (10 columns) - only if not "-"
        if (session.playerId) {
            tui::SelectableElement pidElem;
            pidElem.lineIndex = lineIdx;
            pidElem.startCol = col;
            pidElem.endCol = col + 10;
            pidElem.value = playerIdStr;
            pidElem.truncatedValue = playerIdStr;
            pidElem.type = tui::ElementType::PlayerId;
            pidElem.associatedPlayerId = *session.playerId;
            pidElem.associatedEmail = session.email;
            output.elements.push_back(pidElem);
        }
        col += 10;

        // Endpoint element (22 columns) - only if not "-"
        if (!session.udpEndpoint.empty()) {
            tui::SelectableElement epElem;
            epElem.lineIndex = lineIdx;
            epElem.startCol = col;
            epElem.endCol = col + 22;
            epElem.value = endpointStr;
            epElem.truncatedValue = endpointTrunc;
            epElem.type = tui::ElementType::Endpoint;
            epElem.associatedEmail = session.email;
            if (session.playerId) {
                epElem.associatedPlayerId = *session.playerId;
            }
            output.elements.push_back(epElem);
        }

        lineIdx++;
    }

    output.lines.push_back("╚═══════════════════════════════════════════════════════════════════════════════════════════════╝");
    output.lines.push_back("");

    return output;
}

tui::InteractiveOutput ServerCLI::buildBansInteractiveOutput() {
    tui::InteractiveOutput output;
    output.sourceCommand = "bans";

    auto bannedUsers = _sessionManager->getBannedUsers();
    if (bannedUsers.empty()) {
        return output;
    }

    output.lines.push_back("");
    output.lines.push_back("╔═════════════════════════════════════════════════════════════════════════════════╗");
    output.lines.push_back("║                                  BANNED USERS                                   ║");
    output.lines.push_back("╠═════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(40) << "Email"
           << std::setw(38) << "Display Name" << "  ║";
    output.lines.push_back(header.str());
    output.lines.push_back("╠═════════════════════════════════════════════════════════════════════════════════╣");

    size_t lineIdx = output.lines.size();

    for (const auto& banned : bannedUsers) {
        std::string emailTrunc = tui::utf8::truncateWithEllipsis(banned.email, 39);
        std::string displayNameTrunc = banned.displayName.empty()
            ? "-"
            : tui::utf8::truncateWithEllipsis(banned.displayName, 37);

        std::ostringstream row;
        row << "║ " << std::left << std::setw(40) << emailTrunc
            << std::setw(38) << displayNameTrunc << "  ║";
        output.lines.push_back(row.str());

        // Column positions (after "║ ")
        size_t col = 2;  // Start after "║ "

        // Email element (40 columns)
        tui::SelectableElement emailElem;
        emailElem.lineIndex = lineIdx;
        emailElem.startCol = col;
        emailElem.endCol = col + 40;
        emailElem.value = banned.email;
        emailElem.truncatedValue = emailTrunc;
        emailElem.type = tui::ElementType::Email;
        output.elements.push_back(emailElem);
        col += 40;

        // Display Name element (38 columns) - only if not empty
        if (!banned.displayName.empty()) {
            tui::SelectableElement nameElem;
            nameElem.lineIndex = lineIdx;
            nameElem.startCol = col;
            nameElem.endCol = col + 38;
            nameElem.value = banned.displayName;
            nameElem.truncatedValue = displayNameTrunc;
            nameElem.type = tui::ElementType::DisplayName;
            nameElem.associatedEmail = banned.email;
            output.elements.push_back(nameElem);
        }

        lineIdx++;
    }

    output.lines.push_back("╚═════════════════════════════════════════════════════════════════════════════════╝");
    output.lines.push_back("");

    return output;
}

tui::InteractiveOutput ServerCLI::buildUsersInteractiveOutput() {
    tui::InteractiveOutput output;
    output.sourceCommand = "users";

    if (!_userRepository) {
        return output;
    }

    auto users = _userRepository->findAll();
    if (users.empty()) {
        return output;
    }

    output.lines.push_back("");
    output.lines.push_back("╔═════════════════════════════════════════════════════════════════════════════════╗");
    output.lines.push_back("║                                REGISTERED USERS                                 ║");
    output.lines.push_back("╠═════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(40) << "Email"
           << std::setw(25) << "Username"
           << std::setw(14) << "Status" << " ║";
    output.lines.push_back(header.str());
    output.lines.push_back("╠═════════════════════════════════════════════════════════════════════════════════╣");

    size_t lineIdx = output.lines.size();

    for (const auto& user : users) {
        std::string email = user.getEmail().value();
        std::string username = user.getUsername().value();

        // Check status
        std::string statusStr;
        if (_sessionManager->isBanned(email)) {
            statusStr = "Banned";
        } else if (_sessionManager->hasActiveSession(email)) {
            statusStr = "Online";
        } else {
            statusStr = "Offline";
        }

        // Truncate fields (UTF-8 aware)
        std::string emailTrunc = tui::utf8::truncateWithEllipsis(email, 39);
        std::string usernameTrunc = tui::utf8::truncateWithEllipsis(username, 24);

        std::ostringstream row;
        row << "║ " << std::left << std::setw(40) << emailTrunc
            << std::setw(25) << usernameTrunc
            << std::setw(14) << statusStr << " ║";
        output.lines.push_back(row.str());

        // Column positions (after "║ ")
        size_t col = 2;  // Start after "║ "

        // Email element (40 columns)
        tui::SelectableElement emailElem;
        emailElem.lineIndex = lineIdx;
        emailElem.startCol = col;
        emailElem.endCol = col + 40;
        emailElem.value = email;
        emailElem.truncatedValue = emailTrunc;
        emailElem.type = tui::ElementType::Email;
        output.elements.push_back(emailElem);
        col += 40;

        // Username/DisplayName element (25 columns)
        tui::SelectableElement nameElem;
        nameElem.lineIndex = lineIdx;
        nameElem.startCol = col;
        nameElem.endCol = col + 25;
        nameElem.value = username;
        nameElem.truncatedValue = usernameTrunc;
        nameElem.type = tui::ElementType::DisplayName;
        nameElem.associatedEmail = email;
        output.elements.push_back(nameElem);

        lineIdx++;
    }

    output.lines.push_back("╚═════════════════════════════════════════════════════════════════════════════════╝");
    output.lines.push_back("");

    return output;
}

tui::InteractiveOutput ServerCLI::buildRoomsInteractiveOutput() {
    tui::InteractiveOutput output;
    output.sourceCommand = "rooms";

    if (!_roomManager) {
        return output;
    }

    auto rooms = _roomManager->getAllRooms();
    if (rooms.empty()) {
        return output;
    }

    output.lines.push_back("");
    output.lines.push_back("╔══════════════════════════════════════════════════════════════════════════════════╗");
    output.lines.push_back("║                                   ACTIVE ROOMS                                   ║");
    output.lines.push_back("╠══════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(8) << "Code"
           << std::setw(25) << "Name"
           << std::setw(10) << "Players"
           << std::setw(12) << "State"
           << std::setw(10) << "Private"
           << std::setw(15) << "Host" << " ║";
    output.lines.push_back(header.str());
    output.lines.push_back("╠══════════════════════════════════════════════════════════════════════════════════╣");

    size_t lineIdx = output.lines.size();

    for (const auto* room : rooms) {
        std::string stateStr;
        switch (room->getState()) {
            case domain::entities::Room::State::Waiting: stateStr = "Waiting"; break;
            case domain::entities::Room::State::Starting: stateStr = "Starting"; break;
            case domain::entities::Room::State::InGame: stateStr = "InGame"; break;
            case domain::entities::Room::State::Closed: stateStr = "Closed"; break;
        }

        std::string playersStr = std::to_string(room->getPlayerCount()) + "/" +
                                 std::to_string(room->getMaxPlayers());

        // Truncate fields (UTF-8 aware)
        std::string codeTrunc = tui::utf8::truncateWithEllipsis(room->getCode(), 7);
        std::string nameTrunc = tui::utf8::truncateWithEllipsis(room->getName(), 24);
        std::string hostTrunc = tui::utf8::truncateWithEllipsis(room->getHostEmail(), 14);

        std::ostringstream row;
        row << "║ " << std::left << std::setw(8) << codeTrunc
            << std::setw(25) << nameTrunc
            << std::setw(10) << playersStr
            << std::setw(12) << stateStr
            << std::setw(10) << (room->isPrivate() ? "Yes" : "No")
            << std::setw(15) << hostTrunc << " ║";
        output.lines.push_back(row.str());

        // Column positions (after "║ ")
        size_t col = 2;  // Start after "║ "

        // Code element (8 columns)
        tui::SelectableElement codeElem;
        codeElem.lineIndex = lineIdx;
        codeElem.startCol = col;
        codeElem.endCol = col + 8;
        codeElem.value = room->getCode();
        codeElem.truncatedValue = codeTrunc;
        codeElem.type = tui::ElementType::RoomCode;
        output.elements.push_back(codeElem);
        col += 8;

        // Name element (25 columns)
        tui::SelectableElement nameElem;
        nameElem.lineIndex = lineIdx;
        nameElem.startCol = col;
        nameElem.endCol = col + 25;
        nameElem.value = room->getName();
        nameElem.truncatedValue = nameTrunc;
        nameElem.type = tui::ElementType::RoomName;
        nameElem.associatedRoomCode = room->getCode();
        output.elements.push_back(nameElem);
        col += 25;

        // Skip Players and State (not selectable)
        col += 10 + 12;

        // Skip Private (not selectable)
        col += 10;

        // Host element (15 columns)
        tui::SelectableElement hostElem;
        hostElem.lineIndex = lineIdx;
        hostElem.startCol = col;
        hostElem.endCol = col + 15;
        hostElem.value = room->getHostEmail();
        hostElem.truncatedValue = hostTrunc;
        hostElem.type = tui::ElementType::Email;
        hostElem.associatedRoomCode = room->getCode();
        output.elements.push_back(hostElem);

        lineIdx++;
    }

    output.lines.push_back("╚══════════════════════════════════════════════════════════════════════════════════╝");
    output.lines.push_back("");

    return output;
}

tui::InteractiveOutput ServerCLI::buildRoomDetailsInteractiveOutput(const std::string& roomCode) {
    tui::InteractiveOutput output;
    output.sourceCommand = "room";

    if (!_roomManager) return output;

    auto* room = _roomManager->getRoomByCode(roomCode);
    if (!room) return output;

    size_t lineIdx = 0;

    // State string
    std::string stateStr;
    switch (room->getState()) {
        case domain::entities::Room::State::Waiting: stateStr = "Waiting"; break;
        case domain::entities::Room::State::Starting: stateStr = "Starting"; break;
        case domain::entities::Room::State::InGame: stateStr = "InGame"; break;
        case domain::entities::Room::State::Closed: stateStr = "Closed"; break;
    }

    // Lines 0-3: Header
    output.lines.push_back("");
    output.lines.push_back("╔═════════════════════════════════════════════════════════════════╗");
    output.lines.push_back("║                         ROOM DETAILS                            ║");
    output.lines.push_back("╠═════════════════════════════════════════════════════════════════╣");
    lineIdx = 4;

    // Line 4: Code (selectable - RoomCode)
    std::ostringstream oss;
    oss << "║ Code:       " << std::left << std::setw(52) << room->getCode() << "║";
    output.lines.push_back(oss.str());

    tui::SelectableElement codeElem;
    codeElem.lineIndex = lineIdx;
    codeElem.startCol = 14;  // After "║ Code:       "
    codeElem.endCol = 14 + 52;
    codeElem.value = room->getCode();
    codeElem.truncatedValue = room->getCode();
    codeElem.type = tui::ElementType::RoomCode;
    output.elements.push_back(codeElem);
    lineIdx++;

    // Line 5: Name (not selectable)
    oss.str("");
    oss << "║ Name:       " << std::left << std::setw(52) << room->getName() << "║";
    output.lines.push_back(oss.str());
    lineIdx++;

    // Line 6: Host (selectable - Email)
    std::string hostTrunc = tui::utf8::truncateWithEllipsis(room->getHostEmail(), 51);
    oss.str("");
    oss << "║ Host:       " << std::left << std::setw(52) << hostTrunc << "║";
    output.lines.push_back(oss.str());

    tui::SelectableElement hostElem;
    hostElem.lineIndex = lineIdx;
    hostElem.startCol = 14;
    hostElem.endCol = 14 + 52;
    hostElem.value = room->getHostEmail();
    hostElem.truncatedValue = hostTrunc;
    hostElem.type = tui::ElementType::Email;
    hostElem.associatedRoomCode = room->getCode();
    output.elements.push_back(hostElem);
    lineIdx++;

    // Line 7: Players count (not selectable)
    std::string playersStr = std::to_string(room->getPlayerCount()) + "/" +
                             std::to_string(room->getMaxPlayers());
    oss.str("");
    oss << "║ Players:    " << std::left << std::setw(52) << playersStr << "║";
    output.lines.push_back(oss.str());
    lineIdx++;

    // Line 8: State (not selectable)
    oss.str("");
    oss << "║ State:      " << std::left << std::setw(52) << stateStr << "║";
    output.lines.push_back(oss.str());
    lineIdx++;

    // Line 9: Private (not selectable)
    oss.str("");
    oss << "║ Private:    " << std::left << std::setw(52) << (room->isPrivate() ? "Yes" : "No") << "║";
    output.lines.push_back(oss.str());
    lineIdx++;

    // Players section header
    output.lines.push_back("╠═════════════════════════════════════════════════════════════════╣");
    output.lines.push_back("║                             PLAYERS                             ║");
    output.lines.push_back("╠═════════════════════════════════════════════════════════════════╣");
    lineIdx += 3;

    // Player table header
    std::ostringstream playerHeader;
    playerHeader << "║ " << std::left << std::setw(6) << "Slot"
                 << std::setw(20) << "Display Name"
                 << std::setw(10) << "Ready"
                 << std::setw(10) << "Host"
                 << std::setw(18) << "Email" << "║";
    output.lines.push_back(playerHeader.str());
    lineIdx++;

    output.lines.push_back("╠═════════════════════════════════════════════════════════════════╣");
    lineIdx++;

    // Player rows (selectable: DisplayName and Email)
    const auto& slots = room->getSlots();
    for (size_t i = 0; i < domain::entities::Room::MAX_SLOTS; ++i) {
        if (slots[i].occupied) {
            std::string nameTrunc = tui::utf8::truncateWithEllipsis(slots[i].displayName, 19);
            std::string emailTrunc = tui::utf8::truncateWithEllipsis(slots[i].email, 17);

            std::ostringstream playerRow;
            playerRow << "║ " << std::left << std::setw(6) << i
                      << std::setw(20) << nameTrunc
                      << std::setw(10) << (slots[i].isReady ? "Yes" : "No")
                      << std::setw(10) << (slots[i].isHost ? "Yes" : "No")
                      << std::setw(18) << emailTrunc << "║";
            output.lines.push_back(playerRow.str());

            // Column positions: "║ " = 2, Slot = 6, DisplayName = 20, Ready = 10, Host = 10, Email = 18
            size_t col = 2;

            // Slot (not selectable)
            col += 6;

            // DisplayName (selectable)
            tui::SelectableElement nameElem;
            nameElem.lineIndex = lineIdx;
            nameElem.startCol = col;
            nameElem.endCol = col + 20;
            nameElem.value = slots[i].displayName;
            nameElem.truncatedValue = nameTrunc;
            nameElem.type = tui::ElementType::DisplayName;
            nameElem.associatedEmail = slots[i].email;
            nameElem.associatedRoomCode = room->getCode();
            output.elements.push_back(nameElem);
            col += 20;

            // Ready (not selectable)
            col += 10;

            // Host (not selectable)
            col += 10;

            // Email (selectable)
            tui::SelectableElement emailElem;
            emailElem.lineIndex = lineIdx;
            emailElem.startCol = col;
            emailElem.endCol = col + 18;
            emailElem.value = slots[i].email;
            emailElem.truncatedValue = emailTrunc;
            emailElem.type = tui::ElementType::Email;
            emailElem.associatedRoomCode = room->getCode();
            output.elements.push_back(emailElem);

            lineIdx++;
        }
    }

    // Chat history section (not selectable)
    const auto& chatHistory = room->getChatHistory();
    if (!chatHistory.empty()) {
        output.lines.push_back("╠═════════════════════════════════════════════════════════════════╣");
        output.lines.push_back("║                        CHAT HISTORY                             ║");
        output.lines.push_back("╠═════════════════════════════════════════════════════════════════╣");
        lineIdx += 3;

        size_t startIdx = chatHistory.size() > 10 ? chatHistory.size() - 10 : 0;
        for (size_t i = startIdx; i < chatHistory.size(); ++i) {
            const auto& msg = chatHistory[i];
            std::string senderTrunc = tui::utf8::truncateWithEllipsis(msg.displayName, 12);
            std::string msgTrunc = tui::utf8::truncateWithEllipsis(msg.message, 48);

            std::ostringstream chatRow;
            chatRow << "║ " << std::left << std::setw(14) << ("[" + senderTrunc + "]")
                    << std::setw(50) << msgTrunc << " ║";
            output.lines.push_back(chatRow.str());
            lineIdx++;
        }

        if (startIdx > 0) {
            std::ostringstream moreRow;
            moreRow << "║ " << std::left << std::setw(62)
                    << ("... " + std::to_string(startIdx) + " more message(s)") << " ║";
            output.lines.push_back(moreRow.str());
            lineIdx++;
        }
    }

    output.lines.push_back("╚═════════════════════════════════════════════════════════════════╝");
    output.lines.push_back("");

    return output;
}

tui::InteractiveOutput ServerCLI::buildUserDetailsInteractiveOutput(const std::string& email) {
    tui::InteractiveOutput output;
    output.sourceCommand = "user";

    if (!_userRepository) return output;

    auto userOpt = _userRepository->findByEmail(email);
    if (!userOpt) return output;

    const auto& user = *userOpt;
    std::string userEmail = user.getEmail().value();
    std::string username = user.getUsername().value();
    std::string passwordHash = user.getPasswordHash().value();
    std::string oderId = user.getId().value();

    // Get status from session manager
    std::string statusStr;
    if (_sessionManager->isBanned(userEmail)) {
        statusStr = "Banned";
    } else if (_sessionManager->hasActiveSession(userEmail)) {
        statusStr = "Online";
    } else {
        statusStr = "Offline";
    }

    // Format timestamps
    auto formatTime = [](const std::chrono::system_clock::time_point& tp) -> std::string {
        auto time = std::chrono::system_clock::to_time_t(tp);
        std::tm tm = *std::localtime(&time);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    };

    std::string createdAt = formatTime(user.getCreatedAt());
    std::string lastLogin = formatTime(user.getLastLogin());

    size_t lineIdx = 0;

    // Header lines
    output.lines.push_back("");
    output.lines.push_back("╔═════════════════════════════════════════════════════════════════╗");
    output.lines.push_back("║                         USER DETAILS                            ║");
    output.lines.push_back("╠═════════════════════════════════════════════════════════════════╣");
    lineIdx = 4;

    // Email (selectable - UserField, editable)
    std::ostringstream oss;
    oss << "║ Email:      " << std::left << std::setw(52) << userEmail << "║";
    output.lines.push_back(oss.str());

    tui::SelectableElement emailElem;
    emailElem.lineIndex = lineIdx;
    emailElem.startCol = 14;
    emailElem.endCol = 14 + 52;
    emailElem.value = userEmail;
    emailElem.truncatedValue = userEmail;
    emailElem.type = tui::ElementType::UserField;
    emailElem.fieldName = "email";
    emailElem.associatedEmail = userEmail;
    output.elements.push_back(emailElem);
    lineIdx++;

    // Username (selectable - UserField, editable)
    oss.str("");
    oss << "║ Username:   " << std::left << std::setw(52) << username << "║";
    output.lines.push_back(oss.str());

    tui::SelectableElement usernameElem;
    usernameElem.lineIndex = lineIdx;
    usernameElem.startCol = 14;
    usernameElem.endCol = 14 + 52;
    usernameElem.value = username;
    usernameElem.truncatedValue = username;
    usernameElem.type = tui::ElementType::UserField;
    usernameElem.fieldName = "username";
    usernameElem.associatedEmail = userEmail;
    output.elements.push_back(usernameElem);
    lineIdx++;

    // Password hash (selectable - UserField, editable)
    std::string hashTrunc = tui::utf8::truncateWithEllipsis(passwordHash, 51);
    oss.str("");
    oss << "║ Password:   " << std::left << std::setw(52) << hashTrunc << "║";
    output.lines.push_back(oss.str());

    tui::SelectableElement passwordElem;
    passwordElem.lineIndex = lineIdx;
    passwordElem.startCol = 14;
    passwordElem.endCol = 14 + 52;
    passwordElem.value = passwordHash;
    passwordElem.truncatedValue = hashTrunc;
    passwordElem.type = tui::ElementType::UserField;
    passwordElem.fieldName = "password";
    passwordElem.associatedEmail = userEmail;
    output.elements.push_back(passwordElem);
    lineIdx++;

    // User ID (selectable - Generic, copy only)
    oss.str("");
    oss << "║ User ID:    " << std::left << std::setw(52) << oderId << "║";
    output.lines.push_back(oss.str());

    tui::SelectableElement idElem;
    idElem.lineIndex = lineIdx;
    idElem.startCol = 14;
    idElem.endCol = 14 + 52;
    idElem.value = oderId;
    idElem.truncatedValue = oderId;
    idElem.type = tui::ElementType::Generic;
    idElem.associatedEmail = userEmail;
    output.elements.push_back(idElem);
    lineIdx++;

    // Status (not selectable - display only)
    oss.str("");
    oss << "║ Status:     " << std::left << std::setw(52) << statusStr << "║";
    output.lines.push_back(oss.str());
    lineIdx++;

    // Separator
    output.lines.push_back("╠═════════════════════════════════════════════════════════════════╣");
    lineIdx++;

    // Created (not selectable)
    oss.str("");
    oss << "║ Created:    " << std::left << std::setw(52) << createdAt << "║";
    output.lines.push_back(oss.str());
    lineIdx++;

    // Last Login (not selectable)
    oss.str("");
    oss << "║ Last Login: " << std::left << std::setw(52) << lastLogin << "║";
    output.lines.push_back(oss.str());
    lineIdx++;

    output.lines.push_back("╚═════════════════════════════════════════════════════════════════╝");
    output.lines.push_back("");

    return output;
}

void ServerCLI::handleInteractAction(tui::InteractAction action, const tui::SelectableElement& element) {
    switch (action) {
        case tui::InteractAction::Ban:
            if (element.type == tui::ElementType::Email) {
                banUser(element.value);
            }
            break;

        case tui::InteractAction::Kick:
            // Kick by email (works both in room and in game)
            if (element.type == tui::ElementType::Email) {
                kickPlayer(element.value);
            } else if (element.associatedEmail.has_value()) {
                kickPlayer(*element.associatedEmail);
            }
            break;

        case tui::InteractAction::Unban:
            if (element.type == tui::ElementType::Email) {
                unbanUser(element.value);
            }
            break;

        case tui::InteractAction::Copy:
            copyToClipboard(element.value);
            output("[CLI] Copied: " + element.value);
            break;

        case tui::InteractAction::Insert:
            // Insert value into input buffer
            if (_terminalUI) {
                // This would require adding a method to TerminalUI to insert text
                // For now, just output the value so user can copy it
                output("[CLI] Value: " + element.value);
            }
            break;

        case tui::InteractAction::Close:
            if (element.type == tui::ElementType::RoomCode) {
                closeRoom(element.value);
            }
            break;

        case tui::InteractAction::Details:
            if (element.type == tui::ElementType::RoomCode) {
                showRoom(element.value);
            } else if (element.type == tui::ElementType::Email) {
                showUser(element.value);
            }
            break;

        case tui::InteractAction::Edit:
            // Edit is handled directly by TerminalUI's edit mode
            // This case should not be reached
            break;

        default:
            break;
    }
}

void ServerCLI::handleEditConfirm(const tui::SelectableElement& element,
                                   const std::string& newValue) {
    if (!element.associatedEmail.has_value()) {
        output("[CLI] Error: No associated email for edit");
        return;
    }

    if (!_userRepository) {
        output("[CLI] Error: User repository not available");
        return;
    }

    std::string email = *element.associatedEmail;
    std::string fieldName = element.fieldName.value_or("unknown");

    // Get the current user
    auto userOpt = _userRepository->findByEmail(email);
    if (!userOpt) {
        output("[CLI] Error: User not found: " + email);
        return;
    }

    const auto& oldUser = *userOpt;

    try {
        // Create new user with modified field
        if (fieldName == "email") {
            domain::value_objects::user::Email newEmail(newValue);
            domain::entities::User newUser(
                oldUser.getId(),
                oldUser.getUsername(),
                newEmail,
                oldUser.getPasswordHash(),
                oldUser.getLastLogin(),
                oldUser.getCreatedAt()
            );
            _userRepository->update(newUser);
            output("[CLI] Email updated: " + email + " -> " + newValue);

        } else if (fieldName == "username") {
            domain::value_objects::user::Username newUsername(newValue);
            domain::entities::User newUser(
                oldUser.getId(),
                newUsername,
                oldUser.getEmail(),
                oldUser.getPasswordHash(),
                oldUser.getLastLogin(),
                oldUser.getCreatedAt()
            );
            _userRepository->update(newUser);
            output("[CLI] Username updated for " + email);

        } else if (fieldName == "password") {
            // If newValue is a SHA-256 hash (64 hex chars), use it directly
            // Otherwise, validate and hash the password
            std::string passwordHash = newValue;
            if (newValue.length() < 64) {
                // Validate password length BEFORE hashing (min 6 chars)
                /* if (newValue.length() < 6) { // @TEMP @TODO -> For debug purposes only
                    throw domain::exceptions::user::PasswordException(newValue);
                } */
                passwordHash = domain::value_objects::user::utils::hashPassword(newValue);
            }

            domain::value_objects::user::Password newPassword(passwordHash);
            domain::entities::User newUser(
                oldUser.getId(),
                oldUser.getUsername(),
                oldUser.getEmail(),
                newPassword,
                oldUser.getLastLogin(),
                oldUser.getCreatedAt()
            );
            _userRepository->update(newUser);
            output("[CLI] Password updated for " + email);

        } else {
            output("[CLI] Error: Unknown field: " + fieldName);
            return;
        }

        // Refresh user details display
        // Use the new email if that's what was modified
        std::string refreshEmail = (fieldName == "email") ? newValue : email;
        showUser(refreshEmail);

    } catch (const std::exception& e) {
        output("[CLI] Error updating user: " + std::string(e.what()));
    }
}

void ServerCLI::copyToClipboard(const std::string& text) {
#ifdef _WIN32
    // Windows implementation
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hg) {
        memcpy(GlobalLock(hg), text.c_str(), text.size() + 1);
        GlobalUnlock(hg);
        SetClipboardData(CF_TEXT, hg);
    }
    CloseClipboard();
#else
    // Linux/macOS implementation using xclip or wl-copy
    FILE* pipe = popen("xclip -selection clipboard 2>/dev/null", "w");
    if (pipe) {
        fwrite(text.c_str(), 1, text.size(), pipe);
        if (pclose(pipe) == 0) return;
    }
    // Fallback to wl-copy (Wayland)
    pipe = popen("wl-copy 2>/dev/null", "w");
    if (pipe) {
        fwrite(text.c_str(), 1, text.size(), pipe);
        pclose(pipe);
    }
#endif
}

// ============================================================================
// Network Monitor Command
// ============================================================================

void ServerCLI::cmdNet(const std::string& args) {
    (void)args;  // Unused
    if (!_terminalUI) {
        output("[CLI] Network monitor not available (TUI not initialized).");
        return;
    }

    // Set up the network monitor callback
    _terminalUI->setNetworkMonitorCallback([this]() {
        return buildNetworkGraph();
    });

    // Enter network monitor mode
    _terminalUI->enterNetworkMonitorMode();
}

namespace {
    // ANSI color codes
    constexpr const char* COLOR_GREEN = "\033[32m";
    constexpr const char* COLOR_YELLOW = "\033[33m";
    constexpr const char* COLOR_RED = "\033[31m";
    constexpr const char* COLOR_GRAY = "\033[90m";
    constexpr const char* COLOR_CYAN = "\033[36m";
    constexpr const char* COLOR_BOLD = "\033[1m";
    constexpr const char* COLOR_RESET = "\033[0m";

    const char* getRttColor(uint32_t rttMs) {
        if (rttMs < 50) return COLOR_GREEN;
        if (rttMs < 100) return COLOR_YELLOW;
        return COLOR_RED;
    }

    std::string formatBandwidth(double bytesPerSec) {
        double kbps = bytesPerSec / 1024.0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << kbps;
        return oss.str();
    }
}

std::string ServerCLI::buildNetworkGraph() {
    std::ostringstream graph;

    auto networkStats = _udpServer.getNetworkStats();
    if (!networkStats) {
        graph << "  Network statistics not available\n";
        return graph.str();
    }

    // Header
    graph << "\n";
    graph << COLOR_BOLD << "  ══════════════════════════════════════════════════════════════\n" << COLOR_RESET;
    graph << COLOR_BOLD << "                       NETWORK MONITOR\n" << COLOR_RESET;
    graph << COLOR_BOLD << "  ══════════════════════════════════════════════════════════════\n" << COLOR_RESET;
    graph << "\n";

    // Global server stats
    double outCurrent = networkStats->getCurrentSendRate();
    double outAvg = networkStats->getAverageSendRate();
    double inCurrent = networkStats->getCurrentReceiveRate();
    double inAvg = networkStats->getAverageReceiveRate();
    uint32_t globalRtt = networkStats->getGlobalAverageRTT();

    graph << "  " << COLOR_CYAN << "●" << COLOR_RESET << " " << COLOR_BOLD << "SERVER" << COLOR_RESET << " (UDP:4124)\n";
    graph << "  │ ↑ OUT: " << formatBandwidth(outCurrent) << "/" << formatBandwidth(outAvg) << " KB/s (cur/avg)\n";
    graph << "  │ ↓ IN:  " << formatBandwidth(inCurrent) << "/" << formatBandwidth(inAvg) << " KB/s\n";
    graph << "  │ RTT avg: " << getRttColor(globalRtt) << globalRtt << " ms" << COLOR_RESET << "\n";
    graph << "  │\n";

    // Collect all endpoints that are in rooms
    std::set<std::string> endpointsInRooms;

    // Get all rooms and their players
    if (_roomManager) {
        auto rooms = _roomManager->getAllRooms();
        bool isLastRoom = false;

        for (size_t roomIdx = 0; roomIdx < rooms.size(); ++roomIdx) {
            const auto* room = rooms[roomIdx];
            isLastRoom = (roomIdx == rooms.size() - 1);
            std::string roomPrefix = isLastRoom ? "  └─" : "  ├─";
            std::string childPrefix = isLastRoom ? "    " : "  │ ";

            // Collect all players in this room (with or without UDP)
            std::vector<std::string> roomEndpoints;  // Only UDP-connected players
            struct RoomPlayerInfo {
                std::string displayName;
                std::string endpoint;  // Empty if not UDP-bound
                bool hasUdp;
            };
            std::vector<RoomPlayerInfo> playersInfo;

            for (const auto& slot : room->getSlots()) {
                if (slot.occupied) {
                    auto session = _sessionManager->getSessionByEmail(slot.email);
                    if (session && session->udpBound && !session->udpEndpoint.empty()) {
                        roomEndpoints.push_back(session->udpEndpoint);
                        endpointsInRooms.insert(session->udpEndpoint);
                        playersInfo.push_back({slot.displayName, session->udpEndpoint, true});
                    } else {
                        // Player in room but no UDP connection yet (in lobby)
                        playersInfo.push_back({slot.displayName, "", false});
                    }
                }
            }

            // Get room aggregate stats
            auto roomStats = networkStats->getRoomStats(roomEndpoints);

            // Room header
            if (playersInfo.empty()) {
                // Empty room (no players at all)
                graph << roomPrefix << COLOR_GRAY << "○" << COLOR_RESET << " "
                      << COLOR_GRAY << "ROOM: " << room->getCode() << " \"" << room->getName()
                      << "\" (empty)" << COLOR_RESET << "\n";
            } else if (roomEndpoints.empty()) {
                // Room with players but no UDP connections (all in lobby)
                graph << roomPrefix << COLOR_YELLOW << "○" << COLOR_RESET << " "
                      << COLOR_BOLD << "ROOM: " << room->getCode() << COLOR_RESET
                      << " \"" << room->getName() << "\" (" << playersInfo.size() << " in lobby)\n";
                graph << childPrefix << COLOR_GRAY << "   (waiting to start)" << COLOR_RESET << "\n";

                // Show players in lobby
                if (!_terminalUI->areRoomsCollapsed()) {
                    for (size_t playerIdx = 0; playerIdx < playersInfo.size(); ++playerIdx) {
                        const auto& player = playersInfo[playerIdx];
                        bool isLastPlayer = (playerIdx == playersInfo.size() - 1);
                        std::string playerPrefix = isLastPlayer ? "└──" : "├──";
                        graph << childPrefix << playerPrefix << " " << COLOR_GRAY
                              << player.displayName << " (in lobby)" << COLOR_RESET << "\n";
                    }
                }
            } else {
                // Room with UDP players (game running)
                const char* roomRttColor = getRttColor(roomStats.rttAverage);
                graph << roomPrefix << COLOR_CYAN << "●" << COLOR_RESET << " "
                      << COLOR_BOLD << "ROOM: " << room->getCode() << COLOR_RESET
                      << " \"" << room->getName() << "\" (" << playersInfo.size() << " players)\n";
                graph << childPrefix << "│ ↑ OUT: " << formatBandwidth(roomStats.outCurrent) << "/"
                      << formatBandwidth(roomStats.outAverage) << " KB/s (cur/avg)\n";
                graph << childPrefix << "│ ↓ IN:  " << formatBandwidth(roomStats.inCurrent) << "/"
                      << formatBandwidth(roomStats.inAverage) << " KB/s\n";
                graph << childPrefix << "│ RTT avg: " << roomRttColor << roomStats.rttAverage << " ms" << COLOR_RESET << "\n";
                graph << childPrefix << "│\n";

                // Players in room (only if not collapsed)
                if (!_terminalUI->areRoomsCollapsed()) {
                    for (size_t playerIdx = 0; playerIdx < playersInfo.size(); ++playerIdx) {
                        const auto& player = playersInfo[playerIdx];
                        bool isLastPlayer = (playerIdx == playersInfo.size() - 1);
                        std::string playerPrefix = isLastPlayer ? "└──" : "├──";
                        std::string playerChildPrefix = isLastPlayer ? "   " : "│  ";

                        if (player.hasUdp) {
                            // Player with UDP connection - show full stats
                            auto playerStats = networkStats->getPlayerStats(player.endpoint);
                            if (playerStats) {
                                const char* rttColor = getRttColor(playerStats->rttCurrent);
                                graph << childPrefix << playerPrefix << " " << player.displayName
                                      << " (" << player.endpoint << ")\n";
                                graph << childPrefix << playerChildPrefix << "   RTT: "
                                      << rttColor << playerStats->rttCurrent << "/" << playerStats->rttAverage
                                      << "/" << playerStats->rttMax << " ms" << COLOR_RESET
                                      << "  ↑ " << formatBandwidth(playerStats->outCurrent) << "/"
                                      << formatBandwidth(playerStats->outAverage) << "/"
                                      << formatBandwidth(playerStats->outPeak) << " KB/s"
                                      << "  ↓ " << formatBandwidth(playerStats->inCurrent) << "/"
                                      << formatBandwidth(playerStats->inAverage) << " KB/s\n";
                            }
                        } else {
                            // Player in lobby (no UDP yet)
                            graph << childPrefix << playerPrefix << " " << COLOR_GRAY
                                  << player.displayName << " (in lobby)" << COLOR_RESET << "\n";
                        }
                    }
                } else {
                    graph << childPrefix << "   " << COLOR_GRAY << "(" << playersInfo.size()
                          << " players - press 'c' to expand)" << COLOR_RESET << "\n";
                }
            }

            if (!isLastRoom) {
                graph << "  │\n";
            }
        }
    }

    // Connected users not in a room (TCP only, no UDP yet)
    std::vector<session::Session> usersNotInRoom;
    auto allSessions = _sessionManager->getAllSessions();

    for (const auto& sess : allSessions) {
        // Check if user is in any room
        bool inRoom = false;
        if (_roomManager) {
            inRoom = _roomManager->isPlayerInRoom(sess.email);
        }
        if (!inRoom) {
            usersNotInRoom.push_back(sess);
        }
    }

    if (!usersNotInRoom.empty()) {
        graph << "  │\n";
        graph << "  ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄\n";
        graph << "  " << COLOR_YELLOW << "○" << COLOR_RESET << " "
              << COLOR_BOLD << "CONNECTED USERS" << COLOR_RESET
              << " (not in a room - TCP only)\n";
        graph << "  │\n";

        for (size_t i = 0; i < usersNotInRoom.size(); ++i) {
            const auto& sess = usersNotInRoom[i];
            bool isLast = (i == usersNotInRoom.size() - 1);
            std::string prefix = isLast ? "  └──" : "  ├──";

            graph << prefix << " " << sess.displayName << " (" << sess.email << ")\n";
        }
    }

    // Footer with legend
    graph << "\n";
    graph << "  ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄\n";
    graph << "  RTT colors: " << COLOR_GREEN << "<50ms" << COLOR_RESET << " │ "
          << COLOR_YELLOW << "50-100ms" << COLOR_RESET << " │ "
          << COLOR_RED << ">100ms" << COLOR_RESET << "\n";

    return graph.str();
}

// ============================================================================
// Private Message Admin Commands
// ============================================================================

void ServerCLI::pmStats(const std::string& /*args*/) {
    if (!_pmRepository) {
        output("[PM] Private message repository not available");
        return;
    }

    auto [totalMessages, totalConversations] = _pmRepository->getMessageStats();

    output("");
    output("╔═════════════════════════════════════╗");
    output("║       PRIVATE MESSAGE STATS         ║");
    output("╠═════════════════════════════════════╣");

    std::ostringstream oss;
    oss << "║ Total messages:     " << std::setw(15) << totalMessages << " ║";
    output(oss.str());

    oss.str("");
    oss << "║ Total conversations:" << std::setw(15) << totalConversations << " ║";
    output(oss.str());

    output("╚═════════════════════════════════════╝");
    output("");
}

void ServerCLI::pmUser(const std::string& args) {
    if (!_pmRepository) {
        output("[PM] Private message repository not available");
        return;
    }

    auto parsedArgs = parseArgs(args);
    if (parsedArgs.empty()) {
        output("[PM] Usage: pmuser <email>");
        return;
    }

    const std::string& email = parsedArgs[0];
    size_t limit = parsedArgs.size() > 1 ? std::stoul(parsedArgs[1]) : 100;

    auto messages = _pmRepository->getMessagesByUser(email, limit);

    output("");
    output("╔═══════════════════════════════════════════════════════════════════════╗");
    output("║                    MESSAGES FOR USER: " + email);
    output("╠═══════════════════════════════════════════════════════════════════════╣");

    if (messages.empty()) {
        output("║ No messages found                                                     ║");
    } else {
        for (const auto& msg : messages) {
            std::ostringstream oss;

            // Format timestamp
            auto timeT = std::chrono::system_clock::to_time_t(msg.timestamp);
            std::tm tm{};
            localtime_r(&timeT, &tm);
            char timeBuf[32];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", &tm);

            // Direction indicator
            std::string direction = (msg.senderEmail == email) ? "→" : "←";
            std::string otherParty = (msg.senderEmail == email) ? msg.recipientEmail : msg.senderEmail;
            std::string readStatus = msg.isRead ? "✓" : "○";

            oss << "║ [" << timeBuf << "] " << readStatus << " "
                << direction << " " << otherParty;
            output(oss.str());

            // Truncate message if too long
            std::string msgContent = msg.message;
            if (msgContent.length() > 60) {
                msgContent = msgContent.substr(0, 57) + "...";
            }
            output("║   \"" + msgContent + "\"");
            output("║");
        }
    }

    output("╠═══════════════════════════════════════════════════════════════════════╣");
    std::ostringstream footer;
    footer << "║ Showing " << messages.size() << " messages (limit: " << limit << ")";
    output(footer.str());
    output("╚═══════════════════════════════════════════════════════════════════════╝");
    output("");
}

void ServerCLI::pmConversation(const std::string& args) {
    if (!_pmRepository) {
        output("[PM] Private message repository not available");
        return;
    }

    auto parsedArgs = parseArgs(args);
    if (parsedArgs.size() < 2) {
        output("[PM] Usage: pmconv <email1> <email2> [limit]");
        return;
    }

    const std::string& email1 = parsedArgs[0];
    const std::string& email2 = parsedArgs[1];
    size_t limit = parsedArgs.size() > 2 ? std::stoul(parsedArgs[2]) : 50;

    auto messages = _pmRepository->getConversation(email1, email2, 0, limit);

    output("");
    output("╔═══════════════════════════════════════════════════════════════════════╗");
    output("║         CONVERSATION: " + email1 + " <-> " + email2);
    output("╠═══════════════════════════════════════════════════════════════════════╣");

    if (messages.empty()) {
        output("║ No messages found                                                     ║");
    } else {
        for (const auto& msg : messages) {
            std::ostringstream oss;

            // Format timestamp
            auto timeT = std::chrono::system_clock::to_time_t(msg.timestamp);
            std::tm tm{};
            localtime_r(&timeT, &tm);
            char timeBuf[32];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", &tm);

            std::string readStatus = msg.isRead ? "✓" : "○";
            std::string sender = msg.senderDisplayName.empty() ? msg.senderEmail : msg.senderDisplayName;

            oss << "║ [" << timeBuf << "] " << readStatus << " " << sender << ":";
            output(oss.str());

            // Truncate message if too long
            std::string msgContent = msg.message;
            if (msgContent.length() > 60) {
                msgContent = msgContent.substr(0, 57) + "...";
            }
            output("║   \"" + msgContent + "\"");
            output("║");
        }
    }

    output("╠═══════════════════════════════════════════════════════════════════════╣");
    std::ostringstream footer;
    footer << "║ Showing " << messages.size() << " messages";
    output(footer.str());
    output("╚═══════════════════════════════════════════════════════════════════════╝");
    output("");
}

void ServerCLI::pmSearch(const std::string& args) {
    if (!_pmRepository) {
        output("[PM] Private message repository not available");
        return;
    }

    if (args.empty()) {
        output("[PM] Usage: pmsearch <search_term>");
        return;
    }

    auto messages = _pmRepository->searchMessages(args, 50);

    output("");
    output("╔═══════════════════════════════════════════════════════════════════════╗");
    output("║           SEARCH RESULTS FOR: \"" + args + "\"");
    output("╠═══════════════════════════════════════════════════════════════════════╣");

    if (messages.empty()) {
        output("║ No messages found                                                     ║");
    } else {
        for (const auto& msg : messages) {
            std::ostringstream oss;

            // Format timestamp
            auto timeT = std::chrono::system_clock::to_time_t(msg.timestamp);
            std::tm tm{};
            localtime_r(&timeT, &tm);
            char timeBuf[32];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", &tm);

            std::string sender = msg.senderDisplayName.empty() ? msg.senderEmail : msg.senderDisplayName;

            oss << "║ [" << timeBuf << "] " << sender << " → " << msg.recipientEmail;
            output(oss.str());

            // Truncate message if too long, highlight search term
            std::string msgContent = msg.message;
            if (msgContent.length() > 60) {
                msgContent = msgContent.substr(0, 57) + "...";
            }
            output("║   \"" + msgContent + "\"");
            output("║");
        }
    }

    output("╠═══════════════════════════════════════════════════════════════════════╣");
    std::ostringstream footer;
    footer << "║ Found " << messages.size() << " messages";
    output(footer.str());
    output("╚═══════════════════════════════════════════════════════════════════════╝");
    output("");
}

void ServerCLI::pmRecent(const std::string& args) {
    if (!_pmRepository) {
        output("[PM] Private message repository not available");
        return;
    }

    size_t limit = 50;
    if (!args.empty()) {
        try {
            limit = std::stoul(args);
        } catch (...) {
            output("[PM] Invalid limit. Usage: pmrecent [limit]");
            return;
        }
    }

    auto messages = _pmRepository->getAllMessages(limit, 0);

    output("");
    output("╔═══════════════════════════════════════════════════════════════════════╗");
    output("║                       RECENT PRIVATE MESSAGES                         ║");
    output("╠═══════════════════════════════════════════════════════════════════════╣");

    if (messages.empty()) {
        output("║ No messages found                                                     ║");
    } else {
        for (const auto& msg : messages) {
            std::ostringstream oss;

            // Format timestamp
            auto timeT = std::chrono::system_clock::to_time_t(msg.timestamp);
            std::tm tm{};
            localtime_r(&timeT, &tm);
            char timeBuf[32];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", &tm);

            std::string readStatus = msg.isRead ? "✓" : "○";
            std::string sender = msg.senderDisplayName.empty() ? msg.senderEmail : msg.senderDisplayName;

            oss << "║ [" << timeBuf << "] " << readStatus << " " << sender << " → " << msg.recipientEmail;
            output(oss.str());

            // Truncate message if too long
            std::string msgContent = msg.message;
            if (msgContent.length() > 60) {
                msgContent = msgContent.substr(0, 57) + "...";
            }
            output("║   \"" + msgContent + "\"");
            output("║");
        }
    }

    output("╠═══════════════════════════════════════════════════════════════════════╣");
    std::ostringstream footer;
    footer << "║ Showing " << messages.size() << " most recent messages";
    output(footer.str());
    output("╚═══════════════════════════════════════════════════════════════════════╝");
    output("");
}

} // namespace infrastructure::cli
