/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** TCPAdminServer - Remote admin interface via TCP (JSON-RPC)
*/

#include "infrastructure/adapters/in/network/TCPAdminServer.hpp"
#include "infrastructure/cli/ServerCLI.hpp"
#include "infrastructure/logging/Logger.hpp"
#include <sstream>
#include <iomanip>
#include <thread>

namespace infrastructure::adapters::in::network {

TCPAdminServer::TCPAdminServer(asio::io_context& io_ctx,
                               uint16_t port,
                               std::shared_ptr<cli::ServerCLI> cli)
    : _io_ctx(io_ctx)
    , _acceptor(io_ctx, tcp::endpoint(asio::ip::address_v4::loopback(), port))
    , _port(port)
    , _cli(std::move(cli))
{
    // Set socket options
    // Bind to localhost only (127.0.0.1) for security - not accessible from outside
    _acceptor.set_option(asio::socket_base::reuse_address(true));
}

TCPAdminServer::~TCPAdminServer() {
    try {
        stop();
    } catch (...) {
        // Suppress all exceptions in destructor to prevent std::terminate
        // Logging is not safe here as logger may already be destroyed
    }
}

void TCPAdminServer::start() {
    if (_running.exchange(true)) {
        return;  // Already running
    }

    auto logger = server::logging::Logger::getNetworkLogger();
    logger->info("TCPAdminServer starting on port {}", _port);
    doAccept();
}

void TCPAdminServer::stop() {
    if (!_running.exchange(false)) {
        return;  // Already stopped
    }

    auto logger = server::logging::Logger::getNetworkLogger();
    logger->info("TCPAdminServer stopping...");

    boost::system::error_code ec;
    _acceptor.close(ec);

    // Close all active connections
    {
        std::lock_guard<std::mutex> lock(_connectionsMutex);
        for (auto& socket : _activeConnections) {
            if (socket && socket->is_open()) {
                socket->close(ec);
            }
        }
        _activeConnections.clear();
    }
}

void TCPAdminServer::setAdminToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(_tokenMutex);
    _adminToken = token;
}

void TCPAdminServer::doAccept() {
    if (!_running) return;

    _acceptor.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec && _running) {
                auto logger = server::logging::Logger::getNetworkLogger();

                // Get endpoint safely - client may disconnect between accept and this call
                boost::system::error_code epEc;
                auto endpoint = socket.remote_endpoint(epEc);
                if (epEc) {
                    logger->warn("TCPAdminServer: Client disconnected before handling: {}", epEc.message());
                    if (_running) {
                        doAccept();
                    }
                    return;
                }

                logger->info("TCPAdminServer: New connection from {}:{}",
                         endpoint.address().to_string(), endpoint.port());

                // Handle client in a detached thread to not block acceptor
                std::thread([this, sock = std::move(socket)]() mutable {
                    handleClient(std::move(sock));
                }).detach();
            }

            // Continue accepting
            if (_running) {
                doAccept();
            }
        });
}

void TCPAdminServer::handleClient(tcp::socket socket) {
    // Track connection
    auto socketPtr = std::make_shared<tcp::socket>(std::move(socket));
    {
        std::lock_guard<std::mutex> lock(_connectionsMutex);
        _activeConnections.insert(socketPtr);
    }

    auto logger = server::logging::Logger::getNetworkLogger();

    try {
        boost::asio::streambuf buffer;

        while (_running && socketPtr->is_open()) {
            // Read until newline (each request is a JSON line)
            boost::system::error_code ec;
            [[maybe_unused]] std::size_t bytes = asio::read_until(*socketPtr, buffer, '\n', ec);

            if (ec) {
                if (ec != asio::error::eof) {
                    logger->debug("TCPAdminServer: Read error: {}", ec.message());
                }
                break;
            }

            // Extract the line
            std::istream is(&buffer);
            std::string request;
            std::getline(is, request);

            // Process and send response
            std::string response = processRequest(request);
            response += "\n";

            asio::write(*socketPtr, asio::buffer(response), ec);
            if (ec) {
                logger->debug("TCPAdminServer: Write error: {}", ec.message());
                break;
            }
        }
    } catch (const std::exception& e) {
        logger->error("TCPAdminServer: Client handler error: {}", e.what());
    }

    // Remove from active connections
    {
        std::lock_guard<std::mutex> lock(_connectionsMutex);
        _activeConnections.erase(socketPtr);
    }

    if (socketPtr->is_open()) {
        boost::system::error_code ec;
        socketPtr->close(ec);
    }
}

std::string TCPAdminServer::processRequest(const std::string& request) {
    // Simple JSON parsing (manual to avoid external dependency)
    // Expected format: {"cmd": "...", "args": "...", "token": "..."}

    auto findValue = [&request](const std::string& key) -> std::string {
        std::string searchKey = "\"" + key + "\"";
        auto keyPos = request.find(searchKey);
        if (keyPos == std::string::npos) return "";

        auto colonPos = request.find(':', keyPos);
        if (colonPos == std::string::npos) return "";

        // Find the value (skip whitespace, find opening quote)
        auto valueStart = request.find('"', colonPos + 1);
        if (valueStart == std::string::npos) return "";

        auto valueEnd = request.find('"', valueStart + 1);
        if (valueEnd == std::string::npos) return "";

        return request.substr(valueStart + 1, valueEnd - valueStart - 1);
    };

    std::string token = findValue("token");
    std::string cmd = findValue("cmd");
    std::string args = findValue("args");

    // Validate token
    if (!validateToken(token)) {
        return buildErrorResponse("Unauthorized: Invalid or missing token");
    }

    // Validate command
    if (cmd.empty()) {
        return buildErrorResponse("Missing 'cmd' field");
    }

    // Special handling for dangerous commands via remote
    if (cmd == "quit" || cmd == "exit") {
        return buildErrorResponse("Command 'quit/exit' not allowed via remote admin");
    }

    // Commands that require TUI and don't work well remotely
    if (cmd == "zoom" || cmd == "interact" || cmd == "net") {
        return buildErrorResponse("Command '" + cmd + "' requires local TUI");
    }

    // Build full command string
    std::string fullCommand = cmd;
    if (!args.empty()) {
        fullCommand += " " + args;
    }

    // Execute and capture output
    auto output = executeCommand(cmd, args);

    return buildResponse(true, output);
}

std::vector<std::string> TCPAdminServer::executeCommand(const std::string& cmd,
                                                         const std::string& args) {
    if (!_cli) {
        return {"Error: CLI not available"};
    }

    std::string fullCommand = cmd;
    if (!args.empty()) {
        fullCommand += " " + args;
    }

    auto logger = server::logging::Logger::getNetworkLogger();
    logger->info("TCPAdminServer: Executing remote command: {}", fullCommand);

    return _cli->executeCommandWithOutput(fullCommand);
}

bool TCPAdminServer::validateToken(const std::string& token) const {
    std::lock_guard<std::mutex> lock(_tokenMutex);

    // If no token is set, reject all requests (for security)
    if (_adminToken.empty()) {
        auto logger = server::logging::Logger::getNetworkLogger();
        logger->warn("TCPAdminServer: Admin token not configured - rejecting request");
        return false;
    }

    return token == _adminToken;
}

std::string TCPAdminServer::buildResponse(bool success,
                                           const std::vector<std::string>& output,
                                           const std::string& error) {
    std::ostringstream json;
    json << "{\"success\":" << (success ? "true" : "false");

    // Output array
    json << ",\"output\":[";
    for (size_t i = 0; i < output.size(); ++i) {
        if (i > 0) json << ",";

        // Escape special characters in JSON string
        json << "\"";
        for (char c : output[i]) {
            switch (c) {
                case '"': json << "\\\""; break;
                case '\\': json << "\\\\"; break;
                case '\n': json << "\\n"; break;
                case '\r': json << "\\r"; break;
                case '\t': json << "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 32) {
                        // Control character - skip or encode
                        json << "\\u00" << std::hex << std::setfill('0')
                             << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
                    } else {
                        json << c;
                    }
            }
        }
        json << "\"";
    }
    json << "]";

    if (!error.empty()) {
        json << ",\"error\":\"";
        for (char c : error) {
            if (c == '"') json << "\\\"";
            else if (c == '\\') json << "\\\\";
            else json << c;
        }
        json << "\"";
    }

    json << "}";
    return json.str();
}

std::string TCPAdminServer::buildErrorResponse(const std::string& error) {
    return buildResponse(false, {}, error);
}

} // namespace infrastructure::adapters::in::network
