/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** TCPAdminServer - Remote admin interface via TCP (JSON-RPC)
*/

#ifndef TCPADMINSERVER_HPP_
#define TCPADMINSERVER_HPP_

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <functional>
#include <atomic>

namespace infrastructure::cli {
    class ServerCLI;
}

namespace infrastructure::adapters::in::network {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

/**
 * @brief TCPAdminServer - Remote administration server
 *
 * Provides a JSON-RPC interface for remote administration of the R-Type server.
 * Commands are executed via ServerCLI and output is captured and returned.
 *
 * Protocol:
 * - Each message is a JSON object terminated by newline
 * - Request: {"cmd": "command_name", "args": "optional args", "token": "auth_token"}
 * - Response: {"success": true/false, "output": ["line1", "line2", ...], "error": "if any"}
 */
class TCPAdminServer : public std::enable_shared_from_this<TCPAdminServer> {
public:
    TCPAdminServer(asio::io_context& io_ctx,
                   uint16_t port,
                   std::shared_ptr<cli::ServerCLI> cli);
    ~TCPAdminServer();

    /// Start accepting connections
    void start();

    /// Stop the server
    void stop();

    /// Set the admin token for authentication
    void setAdminToken(const std::string& token);

    /// Get the listening port
    uint16_t getPort() const { return _port; }

private:
    /// Accept new connections
    void doAccept();

    /// Handle a client connection
    void handleClient(tcp::socket socket);

    /// Process a command request
    std::string processRequest(const std::string& request);

    /// Execute a CLI command and capture output
    std::vector<std::string> executeCommand(const std::string& cmd, const std::string& args);

    /// Validate authentication token
    bool validateToken(const std::string& token) const;

    /// Build JSON response
    std::string buildResponse(bool success,
                              const std::vector<std::string>& output,
                              const std::string& error = "");

    /// Build JSON error response
    std::string buildErrorResponse(const std::string& error);

    asio::io_context& _io_ctx;
    tcp::acceptor _acceptor;
    uint16_t _port;
    std::shared_ptr<cli::ServerCLI> _cli;

    std::string _adminToken;
    mutable std::mutex _tokenMutex;

    std::atomic<bool> _running{false};

    // Track active connections for cleanup
    std::unordered_set<std::shared_ptr<tcp::socket>> _activeConnections;
    std::mutex _connectionsMutex;
};

} // namespace infrastructure::adapters::in::network

#endif /* !TCPADMINSERVER_HPP_ */
