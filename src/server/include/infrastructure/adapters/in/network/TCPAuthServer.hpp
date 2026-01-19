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
#include <boost/asio/ssl.hpp>
#include <memory>
#include <optional>
#include <functional>
#include <chrono>
#include <mutex>
#include <set>
#include <map>

#include "Protocol.hpp"

#include "application/ports/out/persistence/IUserRepository.hpp"
#include "application/ports/out/persistence/IUserSettingsRepository.hpp"
#include "application/ports/out/persistence/ILeaderboardRepository.hpp"
#include "application/ports/out/persistence/IFriendshipRepository.hpp"
#include "application/ports/out/persistence/IFriendRequestRepository.hpp"
#include "application/ports/out/persistence/IBlockedUserRepository.hpp"
#include "application/ports/out/persistence/IPrivateMessageRepository.hpp"
#include "application/ports/out/IIdGenerator.hpp"
#include "application/ports/out/ILogger.hpp"
#include "application/use_cases/auth/Login.hpp"
#include "application/use_cases/auth/Register.hpp"
#include "infrastructure/session/SessionManager.hpp"
#include "infrastructure/room/RoomManager.hpp"
#include "infrastructure/social/FriendManager.hpp"

// Domain exceptions for error handling
#include "domain/exceptions/DomainException.hpp"
#include "domain/exceptions/user/UsernameException.hpp"
#include "domain/exceptions/user/EmailException.hpp"
#include "domain/exceptions/user/PasswordException.hpp"
#include "domain/exceptions/user/UsernameAlreadyExistsException.hpp"
#include "domain/exceptions/user/EmailAlreadyExistsException.hpp"

namespace infrastructure::adapters::in::network {
    using boost::asio::ip::tcp;
    namespace ssl = boost::asio::ssl;
    using application::ports::out::persistence::IUserRepository;
    using application::ports::out::persistence::IUserSettingsRepository;
    using application::ports::out::persistence::ILeaderboardRepository;
    using application::ports::out::persistence::IFriendshipRepository;
    using application::ports::out::persistence::IFriendRequestRepository;
    using application::ports::out::persistence::IBlockedUserRepository;
    using application::ports::out::persistence::IPrivateMessageRepository;
    using application::ports::out::persistence::UserSettingsData;
    using application::ports::out::IIdGenerator;
    using application::ports::out::ILogger;
    using domain::entities::User;
    using infrastructure::session::SessionManager;
    using infrastructure::room::RoomManager;
    using infrastructure::social::FriendManager;

    class Session: public std::enable_shared_from_this<Session> {
        private:
            ssl::stream<tcp::socket> _socket;
            char _readBuffer[BUFFER_SIZE];
            std::vector<uint8_t> _accumulator;
            std::optional<User> _user;
            bool _isAuthenticated = false;
            std::function<void(const User&)> _onAuthSuccess;
            std::shared_ptr<IUserRepository> _userRepository;
            std::shared_ptr<IUserSettingsRepository> _userSettingsRepository;
            std::shared_ptr<ILeaderboardRepository> _leaderboardRepository;
            std::shared_ptr<IIdGenerator> _idGenerator;
            std::shared_ptr<ILogger> _logger;
            std::shared_ptr<SessionManager> _sessionManager;
            std::shared_ptr<RoomManager> _roomManager;
            std::shared_ptr<FriendManager> _friendManager;
            std::shared_ptr<IFriendshipRepository> _friendshipRepository;
            std::shared_ptr<IFriendRequestRepository> _friendRequestRepository;
            std::shared_ptr<IBlockedUserRepository> _blockedUserRepository;
            std::shared_ptr<IPrivateMessageRepository> _privateMessageRepository;

            // Session token (valid after successful login)
            std::optional<SessionToken> _sessionToken;

            boost::asio::steady_timer _timeoutTimer;
            std::chrono::steady_clock::time_point _lastActivity;

            // Callback to notify TCPAuthServer when session closes (receives Session* this)
            std::function<void(Session*)> _onClose;

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

            // Helper for successful join (factorizes JoinRoomByCode and QuickJoin)
            void sendJoinSuccessResponse(const RoomManager::JoinResult& result, MessageType ackType);

            // User settings handlers
            void handleGetUserSettings();
            void handleSaveUserSettings(const std::vector<uint8_t>& payload);

            // Chat handlers
            void handleSendChatMessage(const std::vector<uint8_t>& payload);

            // Hidden command handlers
            void handleToggleGodMode(const std::string& email);

            // Friends System handlers
            void handleSendFriendRequest(const std::vector<uint8_t>& payload);
            void handleAcceptFriendRequest(const std::vector<uint8_t>& payload);
            void handleRejectFriendRequest(const std::vector<uint8_t>& payload);
            void handleRemoveFriend(const std::vector<uint8_t>& payload);
            void handleBlockUser(const std::vector<uint8_t>& payload);
            void handleUnblockUser(const std::vector<uint8_t>& payload);
            void handleGetFriendsList(const std::vector<uint8_t>& payload);
            void handleGetFriendRequests();
            void handleGetBlockedUsers();

            // Private Messaging handlers
            void handleSendPrivateMessage(const std::vector<uint8_t>& payload);
            void handleGetConversation(const std::vector<uint8_t>& payload);
            void handleGetConversationsList();
            void handleMarkMessagesRead(const std::vector<uint8_t>& payload);

            // Leaderboard handlers
            void handleGetLeaderboard(const std::vector<uint8_t>& payload);
            void handleGetPlayerStats();
            void handleGetGameHistory();
            void handleGetAchievements();

            // Leaderboard response writers
            void do_write_leaderboard_response(const std::vector<application::ports::out::persistence::LeaderboardEntry>& entries, uint8_t period, uint8_t playerCountFilter, uint32_t yourRank);
            void do_write_player_stats_response(const application::ports::out::persistence::PlayerStats& stats);
            void do_write_game_history_response(const std::vector<application::ports::out::persistence::GameHistoryEntry>& entries);
            void do_write_achievements_response(const std::vector<application::ports::out::persistence::AchievementRecord>& achievements);

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

            // Friends System response writers
            void do_write_friend_request_ack(uint8_t errorCode, const std::string& targetEmail);
            void do_write_friend_request_received(const std::string& fromEmail, const std::string& fromDisplayName);
            void do_write_accept_friend_request_ack(uint8_t errorCode);
            void do_write_friend_request_accepted(const std::string& friendEmail, const std::string& displayName, uint8_t onlineStatus);
            void do_write_reject_friend_request_ack(uint8_t errorCode);
            void do_write_remove_friend_ack(uint8_t errorCode);
            void do_write_friend_removed(const std::string& friendEmail);
            void do_write_block_user_ack(uint8_t errorCode);
            void do_write_unblock_user_ack(uint8_t errorCode);
            void do_write_friends_list(const std::vector<FriendInfoWire>& friends, uint8_t totalCount);
            void do_write_friend_requests(const std::vector<FriendRequestInfoWire>& incoming, const std::vector<FriendRequestInfoWire>& outgoing);
            void do_write_blocked_users(const std::vector<FriendInfoWire>& blockedUsers);
            void do_write_friend_status_changed(const std::string& friendEmail, uint8_t newStatus, const std::string& roomCode);

            // Private Messaging response writers
            void do_write_private_message_ack(uint8_t errorCode, uint64_t messageId);
            void do_write_private_message_received(const std::string& senderEmail, const std::string& senderDisplayName, const std::string& message, uint64_t timestamp);
            void do_write_conversation(const std::vector<PrivateMessageWire>& messages, bool hasMore);
            void do_write_conversations_list(const std::vector<ConversationSummaryWire>& conversations);
            void do_write_mark_messages_read_ack(uint8_t errorCode);
            void do_write_messages_read_notification(const std::string& readerEmail);

            // Helper to get user's current online status
            uint8_t getCurrentOnlineStatus() const;

            // Broadcast to room members
            void broadcastRoomUpdate(domain::entities::Room* room);
            void broadcastGameStarting(domain::entities::Room* room, uint8_t countdown);

            public:
                Session(ssl::stream<tcp::socket> socket,
                    std::shared_ptr<IUserRepository> userRepository,
                    std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
                    std::shared_ptr<ILeaderboardRepository> leaderboardRepository,
                    std::shared_ptr<IIdGenerator> idGenerator,
                    std::shared_ptr<ILogger> logger,
                    std::shared_ptr<SessionManager> sessionManager,
                    std::shared_ptr<RoomManager> roomManager,
                    std::shared_ptr<FriendManager> friendManager,
                    std::shared_ptr<IFriendshipRepository> friendshipRepository,
                    std::shared_ptr<IFriendRequestRepository> friendRequestRepository,
                    std::shared_ptr<IBlockedUserRepository> blockedUserRepository,
                    std::shared_ptr<IPrivateMessageRepository> privateMessageRepository,
                    std::function<void(Session*)> onClose = nullptr);
                ~Session() noexcept;

                void start();

                // Close the session (cancel timer and close socket)
                void close();
        };

    class TCPAuthServer {
            private:
                boost::asio::io_context& _io_ctx;
                ssl::context _sslContext;
                std::string _certFile;
                std::string _keyFile;
                std::shared_ptr<IUserRepository> _userRepository;
                std::shared_ptr<IUserSettingsRepository> _userSettingsRepository;
                std::shared_ptr<ILeaderboardRepository> _leaderboardRepository;
                std::shared_ptr<IIdGenerator> _idGenerator;
                std::shared_ptr<ILogger> _logger;
                std::shared_ptr<SessionManager> _sessionManager;
                std::shared_ptr<RoomManager> _roomManager;
                std::shared_ptr<FriendManager> _friendManager;
                std::shared_ptr<IFriendshipRepository> _friendshipRepository;
                std::shared_ptr<IFriendRequestRepository> _friendRequestRepository;
                std::shared_ptr<IBlockedUserRepository> _blockedUserRepository;
                std::shared_ptr<IPrivateMessageRepository> _privateMessageRepository;
                tcp::acceptor _acceptor;

                // Track active sessions for graceful shutdown
                // Map raw pointer -> shared_ptr for efficient lookup during unregister
                mutable std::mutex _sessionsMutex;
                std::map<Session*, std::shared_ptr<Session>> _activeSessions;

                void initSSLContext();
                void start_accept();

        public:
            TCPAuthServer(
                boost::asio::io_context& io_ctx,
                const std::string& certFile,
                const std::string& keyFile,
                std::shared_ptr<IUserRepository> userRepository,
                std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
                std::shared_ptr<ILeaderboardRepository> leaderboardRepository,
                std::shared_ptr<IIdGenerator> idGenerator,
                std::shared_ptr<ILogger> logger,
                std::shared_ptr<SessionManager> sessionManager,
                std::shared_ptr<RoomManager> roomManager,
                std::shared_ptr<FriendManager> friendManager,
                std::shared_ptr<IFriendshipRepository> friendshipRepository,
                std::shared_ptr<IFriendRequestRepository> friendRequestRepository,
                std::shared_ptr<IBlockedUserRepository> blockedUserRepository,
                std::shared_ptr<IPrivateMessageRepository> privateMessageRepository);
            void start();
            void run();
            void stop();

            // Accessors
            std::shared_ptr<SessionManager> getSessionManager() const { return _sessionManager; }
            std::shared_ptr<RoomManager> getRoomManager() const { return _roomManager; }
            std::shared_ptr<ILeaderboardRepository> getLeaderboardRepository() const { return _leaderboardRepository; }
            std::shared_ptr<FriendManager> getFriendManager() const { return _friendManager; }

            // Session tracking for graceful shutdown
            void registerSession(std::shared_ptr<Session> session);
            void unregisterSession(Session* session);  // Takes raw pointer (called from destructor)
        };
}
#endif /* !TCPAUTHSERVER_HPP_ */
