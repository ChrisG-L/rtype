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

#include "protocol/CommandParser.hpp"
#include "Protocol.hpp"

#include "infrastructure/adapters/out/persistence/MongoDBUserRepository.hpp"
#include "infrastructure/adapters/in/network/execute/Execute.hpp"

// Domain exceptions for error handling
#include "domain/exceptions/DomainException.hpp"
#include "domain/exceptions/user/UsernameException.hpp"
#include "domain/exceptions/user/EmailException.hpp"
#include "domain/exceptions/user/PasswordException.hpp"
#include "domain/exceptions/user/UsernameAlreadyExistsException.hpp"
#include "domain/exceptions/user/EmailAlreadyExistsException.hpp"
#include "domain/exceptions/user/UserAlreadyLoggedException.hpp"
#include "domain/exceptions/persistence/MongoDBException.hpp"

namespace infrastructure::adapters::in::network {
    using boost::asio::ip::tcp;
    using infrastructure::adapters::out::persistence::MongoDBUserRepository;
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

            void do_read();
            void do_write(const MessageType&, const std::string& message);
            void do_write_auth_response(const MessageType& msgType, const AuthResponse& resp);
            void handle_command(const Header&);
            void onLoginSuccess(const User& user);

            public:
                Session(tcp::socket socket,
                    std::shared_ptr<MongoDBUserRepository> userRepository,
                    std::unordered_map<std::string, User>&);
                ~Session();

                std::shared_ptr<MongoDBUserRepository> _userRepository;
                void start();
        };
        
    class TCPAuthServer {
            private:
                boost::asio::io_context& _io_ctx;
                std::shared_ptr<MongoDBUserRepository> _userRepository;
                tcp::acceptor _acceptor;
                std::unordered_map<std::string, User> users;
                void start_accept();

        public:
            TCPAuthServer(boost::asio::io_context& io_ctx, std::shared_ptr<MongoDBUserRepository> userRepository);
            void start();
            void run();
        };
}
#endif /* !TCPAUTHSERVER_HPP_ */
