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
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdio>

#ifdef _WIN32
// WIN32_LEAN_AND_MEAN prevents windows.h from including winsock.h
// which would conflict with winsock2.h already included by Boost.Asio
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace infrastructure::cli {

ServerCLI::ServerCLI(std::shared_ptr<SessionManager> sessionManager,
                     UDPServer& udpServer,
                     std::shared_ptr<tui::LogBuffer> logBuffer,
                     std::shared_ptr<IUserRepository> userRepository,
                     std::shared_ptr<RoomManager> roomManager)
    : _sessionManager(std::move(sessionManager))
    , _udpServer(udpServer)
    , _logBuffer(std::move(logBuffer))
    , _userRepository(std::move(userRepository))
    , _roomManager(std::move(roomManager))
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
    _commands["logs"] = [this](const std::string& args) { toggleLogs(args); };
    _commands["debug"] = [this](const std::string& args) { toggleDebug(args); };
    _commands["zoom"] = [this](const std::string&) { enterZoomMode(); };
    _commands["interact"] = [this](const std::string& args) { enterInteractMode(args); };
    _commands["quit"] = [this](const std::string&) { stop(); };
    _commands["exit"] = [this](const std::string&) { stop(); };

    // Set up interact action callback
    if (_terminalUI) {
        _terminalUI->setInteractActionCallback(
            [this](tui::InteractAction action, const tui::SelectableElement& element) {
                handleInteractAction(action, element);
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
    output("║ help                 - Show this help message                ║");
    output("║ status               - Show server status                    ║");
    output("║ sessions             - List all active sessions              ║");
    output("║ rooms                - List all active rooms                 ║");
    output("║ room <code>          - Show room details + chat history      ║");
    output("║ closeroom <code>     - Force close a room (kicks all)        ║");
    output("║ kickfromroom <code> <email> [reason]                         ║");
    output("║                      - Kick player from room (admin)         ║");
    output("║ users                - List all registered users (DB)        ║");
    output("║ kick <player_id>     - Kick a player by their in-game ID     ║");
    output("║ ban <email>          - Ban a user permanently                ║");
    output("║ unban <email>        - Unban a user                          ║");
    output("║ bans                 - List all banned users                 ║");
    output("║ logs <on|off>        - Enable/disable all server logs        ║");
    output("║ debug <on|off>       - Enable/disable debug logs             ║");
    output("║ zoom                 - Full-screen log view (ESC to exit)    ║");
    output("║ interact [cmd]       - Navigate output (sessions/bans/users/ ║");
    output("║                        rooms)                                ║");
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
    output("╔════════════════════════════════════════════════════════════════════════════════════════════════╗");
    output("║                                        ACTIVE SESSIONS                                         ║");
    output("╠════════════════════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(25) << "Email"
           << std::setw(15) << "Display Name"
           << std::setw(10) << "Status"
           << std::setw(8) << "Room"
           << std::setw(10) << "Player ID"
           << std::setw(22) << "Endpoint" << "  ║";
    output(header.str());
    output("╠════════════════════════════════════════════════════════════════════════════════════════════════╣");

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
            << std::setw(8) << roomCode
            << std::setw(10) << playerIdStr
            << std::setw(22) << endpointStr << "  ║";
        output(row.str());
    }

    output("╚════════════════════════════════════════════════════════════════════════════════════════════════╝");
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
    output("╔═════════════════════════════════════════════════════════════════════════════════╗");
    output("║                                   ACTIVE ROOMS                                   ║");
    output("╠═════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(8) << "Code"
           << std::setw(25) << "Name"
           << std::setw(10) << "Players"
           << std::setw(12) << "State"
           << std::setw(10) << "Private"
           << std::setw(15) << "Host" << " ║";
    output(header.str());
    output("╠═════════════════════════════════════════════════════════════════════════════════╣");

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

    output("╚═════════════════════════════════════════════════════════════════════════════════╝");
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
    output("║                           PLAYERS                               ║");
    output("╠═════════════════════════════════════════════════════════════════╣");

    std::ostringstream playerHeader;
    playerHeader << "║ " << std::left << std::setw(6) << "Slot"
                 << std::setw(20) << "Display Name"
                 << std::setw(10) << "Ready"
                 << std::setw(10) << "Host"
                 << std::setw(18) << "Email" << " ║";
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
                      << std::setw(18) << emailTrunc << " ║";
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
        } else {
            output("[CLI] Unknown interact target: " + args);
            output("[CLI] Valid targets: sessions, bans, users, rooms");
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
    output.lines.push_back("╔════════════════════════════════════════════════════════════════════════════════════════════════╗");
    output.lines.push_back("║                                        ACTIVE SESSIONS                                         ║");
    output.lines.push_back("╠════════════════════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(25) << "Email"
           << std::setw(15) << "Display Name"
           << std::setw(10) << "Status"
           << std::setw(8) << "Room"
           << std::setw(10) << "Player ID"
           << std::setw(22) << "Endpoint" << "  ║";
    output.lines.push_back(header.str());
    output.lines.push_back("╠════════════════════════════════════════════════════════════════════════════════════════════════╣");

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
            << std::setw(8) << roomCode
            << std::setw(10) << playerIdStr
            << std::setw(22) << endpointTrunc << "  ║";
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
        col += 8;

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

    output.lines.push_back("╚════════════════════════════════════════════════════════════════════════════════════════════════╝");
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
    output.lines.push_back("╔═════════════════════════════════════════════════════════════════════════════════╗");
    output.lines.push_back("║                                   ACTIVE ROOMS                                   ║");
    output.lines.push_back("╠═════════════════════════════════════════════════════════════════════════════════╣");

    std::ostringstream header;
    header << "║ " << std::left << std::setw(8) << "Code"
           << std::setw(25) << "Name"
           << std::setw(10) << "Players"
           << std::setw(12) << "State"
           << std::setw(10) << "Private"
           << std::setw(15) << "Host" << " ║";
    output.lines.push_back(header.str());
    output.lines.push_back("╠═════════════════════════════════════════════════════════════════════════════════╣");

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

    output.lines.push_back("╚═════════════════════════════════════════════════════════════════════════════════╝");
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
            if (element.type == tui::ElementType::PlayerId) {
                kickPlayer(element.value);
            } else if (element.associatedPlayerId.has_value()) {
                kickPlayer(std::to_string(*element.associatedPlayerId));
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
            }
            break;

        default:
            break;
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

} // namespace infrastructure::cli
