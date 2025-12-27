/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** TCPAuthServer - Authentication server on port 4125
*/

#ifndef TCPAUTHSERVER_HPP_
#define TCPAUTHSERVER_HPP_

#include <array>
#include <cstdint>
#include <iostream>
#include <boost/asio.hpp>
#include <memory>
#include <optional>
#include <functional>
#include <chrono>

#include "protocol/CommandParser.hpp"
#include "Protocol.hpp"

#include "application/ports/out/persistence/IUserRepository.hpp"
#include "application/ports/out/IIdGenerator.hpp"
#include "application/ports/out/ILogger.hpp"
#include "infrastructure/adapters/in/network/execute/Execute.hpp"

// Domain exceptions for error handling
#include "domain/exceptions/DomainException.hpp"
#include "domain/exceptions/user/UsernameException.hpp"
#include "domain/exceptions/user/EmailException.hpp"
#include "domain/exceptions/user/PasswordException.hpp"
#include "domain/exceptions/user/UsernameAlreadyExistsException.hpp"
#include "domain/exceptions/user/EmailAlreadyExistsException.hpp"
#include "domain/exceptions/user/UserAlreadyLoggedException.hpp"

namespace infrastructure::adapters::in::network {
    using boost::asio::ip::tcp;
    using application::ports::out::persistence::IUserRepository;
    using application::ports::out::IIdGenerator;
    using application::ports::out::ILogger;
    using domain::entities::User;

    class Session: public std::enable_shared_from_this<Session> {
        private:
            tcp::socket _socket;
            char _readBuffer[BUFFER_SIZE];
            std::vector<uint8_t> _accumulator;
            std::unordered_map<std::string, User>& _users;
            std::optional<User> _user;
            bool _isAuthenticated = false;
            std::function<void(const User&)> _onAuthSuccess;
            std::shared_ptr<IUserRepository> _userRepository;
            std::shared_ptr<IIdGenerator> _idGenerator;
            std::shared_ptr<ILogger> _logger;

            boost::asio::steady_timer _timeoutTimer;
            std::chrono::steady_clock::time_point _lastActivity;

            void do_read();
            void do_write(const MessageType&, const std::string& message);
            void do_write_auth_response(const MessageType& msgType, const AuthResponse& resp);
            void do_write_heartbeat_ack();
            void handle_command(const Header&);
            void onLoginSuccess(const User& user);
            void scheduleTimeoutCheck();

            public:
                Session(tcp::socket socket,
                    std::shared_ptr<IUserRepository> userRepository,
                    std::shared_ptr<IIdGenerator> idGenerator,
                    std::shared_ptr<ILogger> logger,
                    std::unordered_map<std::string, User>&);
                ~Session();

                void start();
        };

    class TCPAuthServer {
            private:
                boost::asio::io_context& _io_ctx;
                std::shared_ptr<IUserRepository> _userRepository;
                std::shared_ptr<IIdGenerator> _idGenerator;
                std::shared_ptr<ILogger> _logger;
                tcp::acceptor _acceptor;
                std::unordered_map<std::string, User> users;
                void start_accept();

        public:
            TCPAuthServer(
                boost::asio::io_context& io_ctx,
                std::shared_ptr<IUserRepository> userRepository,
                std::shared_ptr<IIdGenerator> idGenerator,
                std::shared_ptr<ILogger> logger);
            void start();
            void run();
            void stop();
        };
}
#endif /* !TCPAUTHSERVER_HPP_ */
