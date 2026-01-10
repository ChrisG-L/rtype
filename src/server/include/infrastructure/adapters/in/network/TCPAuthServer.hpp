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
#include "application/ports/out/persistence/IUserSettingsRepository.hpp"
#include "application/ports/out/IIdGenerator.hpp"
#include "application/ports/out/ILogger.hpp"
#include "infrastructure/adapters/in/network/execute/Execute.hpp"
#include "infrastructure/session/SessionManager.hpp"
#include "infrastructure/room/RoomManager.hpp"

// Domain exceptions for error handling
#include "domain/exceptions/DomainException.hpp"
#include "domain/exceptions/user/UsernameException.hpp"
#include "domain/exceptions/user/EmailException.hpp"
#include "domain/exceptions/user/PasswordException.hpp"
#include "domain/exceptions/user/UsernameAlreadyExistsException.hpp"
#include "domain/exceptions/user/EmailAlreadyExistsException.hpp"

namespace infrastructure::adapters::in::network {
    using boost::asio::ip::tcp;
    using application::ports::out::persistence::IUserRepository;
    using application::ports::out::persistence::IUserSettingsRepository;
    using application::ports::out::persistence::UserSettingsData;
    using application::ports::out::IIdGenerator;
    using application::ports::out::ILogger;
    using domain::entities::User;
    using infrastructure::session::SessionManager;
    using infrastructure::room::RoomManager;

    class Session: public std::enable_shared_from_this<Session> {
        private:
            tcp::socket _socket;
            char _readBuffer[BUFFER_SIZE];
            std::vector<uint8_t> _accumulator;
            std::optional<User> _user;
            bool _isAuthenticated = false;
            std::function<void(const User&)> _onAuthSuccess;
            std::shared_ptr<IUserRepository> _userRepository;
            std::shared_ptr<IUserSettingsRepository> _userSettingsRepository;
            std::shared_ptr<IIdGenerator> _idGenerator;
            std::shared_ptr<ILogger> _logger;
            std::shared_ptr<SessionManager> _sessionManager;
            std::shared_ptr<RoomManager> _roomManager;

            // Session token (valid after successful login)
            std::optional<SessionToken> _sessionToken;

            boost::asio::steady_timer _timeoutTimer;
            std::chrono::steady_clock::time_point _lastActivity;

            void do_read();
            void do_write(const MessageType&, const std::string& message);
            void do_write_auth_response(const MessageType& msgType, const AuthResponse& resp);
            void do_write_auth_response_with_token(const MessageType& msgType, const AuthResponseWithToken& resp);
            void do_write_heartbeat_ack();
            void handle_command(const Header&);
            void onLoginSuccess(const User& user);
            void scheduleTimeoutCheck();

            // Room message handlers
            void handleCreateRoom(const std::vector<uint8_t>& payload);
            void handleJoinRoomByCode(const std::vector<uint8_t>& payload);
            void handleLeaveRoom();
            void handleSetReady(const std::vector<uint8_t>& payload);
            void handleStartGame();
            void handleKickPlayer(const std::vector<uint8_t>& payload);
            void handleSetRoomConfig(const std::vector<uint8_t>& payload);
            void handleBrowsePublicRooms();
            void handleQuickJoin();

            // User settings handlers
            void handleGetUserSettings();
            void handleSaveUserSettings(const std::vector<uint8_t>& payload);

            // Chat handlers
            void handleSendChatMessage(const std::vector<uint8_t>& payload);

            // Room response writers
            void do_write_create_room_ack(const CreateRoomAck& ack);
            void do_write_join_room_ack(const JoinRoomAck& ack);
            void do_write_join_room_nack(const JoinRoomNack& nack);
            void do_write_leave_room_ack();
            void do_write_set_ready_ack(const SetReadyAck& ack);
            void do_write_start_game_ack();
            void do_write_start_game_nack(const StartGameNack& nack);
            void do_write_room_update(const RoomUpdate& update);
            void do_write_game_starting(const GameStarting& gs);
            void do_write_kick_player_ack();
            void do_write_player_kicked(const PlayerKickedNotification& notif);
            void do_write_set_room_config_ack(bool success);
            void do_write_browse_public_rooms(const BrowsePublicRoomsResponse& resp);
            void do_write_quick_join_nack(const QuickJoinNack& nack);

            // User settings response writers
            void do_write_get_user_settings_response(const GetUserSettingsResponse& resp);
            void do_write_save_user_settings_response(const SaveUserSettingsResponse& resp);

            // Chat response writers
            void do_write_send_chat_message_ack();
            void do_write_chat_message(const ChatMessagePayload& msg);
            void do_write_chat_history(const ChatHistoryResponse& hist);

            // Broadcast to room members
            void broadcastRoomUpdate(domain::entities::Room* room);
            void broadcastGameStarting(domain::entities::Room* room, uint8_t countdown);

            public:
                Session(tcp::socket socket,
                    std::shared_ptr<IUserRepository> userRepository,
                    std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
                    std::shared_ptr<IIdGenerator> idGenerator,
                    std::shared_ptr<ILogger> logger,
                    std::shared_ptr<SessionManager> sessionManager,
                    std::shared_ptr<RoomManager> roomManager);
                ~Session();

                void start();
        };

    class TCPAuthServer {
            private:
                boost::asio::io_context& _io_ctx;
                std::shared_ptr<IUserRepository> _userRepository;
                std::shared_ptr<IUserSettingsRepository> _userSettingsRepository;
                std::shared_ptr<IIdGenerator> _idGenerator;
                std::shared_ptr<ILogger> _logger;
                std::shared_ptr<SessionManager> _sessionManager;
                std::shared_ptr<RoomManager> _roomManager;
                tcp::acceptor _acceptor;
                void start_accept();

        public:
            TCPAuthServer(
                boost::asio::io_context& io_ctx,
                std::shared_ptr<IUserRepository> userRepository,
                std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
                std::shared_ptr<IIdGenerator> idGenerator,
                std::shared_ptr<ILogger> logger,
                std::shared_ptr<SessionManager> sessionManager,
                std::shared_ptr<RoomManager> roomManager);
            void start();
            void run();
            void stop();

            // Accessors
            std::shared_ptr<SessionManager> getSessionManager() const { return _sessionManager; }
            std::shared_ptr<RoomManager> getRoomManager() const { return _roomManager; }
        };
}
#endif /* !TCPAUTHSERVER_HPP_ */
