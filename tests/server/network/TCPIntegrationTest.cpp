/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests d'intégration TCP avec Boost.Asio
*/

#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <functional>
#include <memory>

using boost::asio::ip::tcp;
using namespace std::chrono_literals;

/**
 * @brief Suite de tests d'intégration TCP
 *
 * Ces tests vérifient :
 * - Établissement de connexion TCP
 * - Envoi et réception de données
 * - Gestion des déconnexions
 * - Communication bidirectionnelle
 * - Gestion des erreurs réseau
 *
 * Utilise un serveur echo simple pour les tests.
 */

// ============================================================================
// Serveur Echo Simple pour les Tests
// ============================================================================

/**
 * @brief Serveur echo simple utilisant Boost.Asio
 *
 * Accepte une connexion et renvoie tous les messages reçus.
 */
class TestEchoServer {
public:
    TestEchoServer(std::uint16_t port)
        : _acceptor(_ioContext, tcp::endpoint(tcp::v4(), port))
        , _port(port)
        , _running(false)
        , _clientConnected(false)
    {}

    ~TestEchoServer() {
        stop();
    }

    void start() {
        _running = true;
        _serverThread = std::thread([this]() {
            try {
                startAccept();
                _ioContext.run();
            } catch (const std::exception& e) {
                // Ignore errors on shutdown
            }
        });

        // Attendre que le serveur soit prêt
        std::this_thread::sleep_for(50ms);
    }

    void stop() {
        _running = false;
        _ioContext.stop();

        if (_serverThread.joinable()) {
            _serverThread.join();
        }
    }

    std::uint16_t getPort() const { return _port; }
    bool isClientConnected() const { return _clientConnected; }

    std::string getLastReceived() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_receivedMessages.empty()) return "";
        std::string msg = _receivedMessages.front();
        _receivedMessages.pop();
        return msg;
    }

    void waitForMessage(int timeoutMs = 1000) {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() {
            return !_receivedMessages.empty();
        });
    }

private:
    void startAccept() {
        _acceptor.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec && _running) {
                _clientConnected = true;
                _socket = std::make_unique<tcp::socket>(std::move(socket));
                startRead();
            }
            if (_running) {
                startAccept();
            }
        });
    }

    void startRead() {
        if (!_socket || !_running) return;

        _socket->async_read_some(
            boost::asio::buffer(_buffer, sizeof(_buffer)),
            [this](boost::system::error_code ec, std::size_t bytes) {
                if (!ec && bytes > 0) {
                    std::string received(_buffer, bytes);

                    {
                        std::lock_guard<std::mutex> lock(_mutex);
                        _receivedMessages.push(received);
                    }
                    _cv.notify_all();

                    // Echo back
                    boost::asio::async_write(*_socket,
                        boost::asio::buffer(received),
                        [this](boost::system::error_code, std::size_t) {
                            startRead();
                        });
                } else if (_running) {
                    _clientConnected = false;
                }
            });
    }

    boost::asio::io_context _ioContext;
    tcp::acceptor _acceptor;
    std::unique_ptr<tcp::socket> _socket;
    std::thread _serverThread;
    std::uint16_t _port;
    std::atomic<bool> _running;
    std::atomic<bool> _clientConnected;
    char _buffer[4096];

    std::mutex _mutex;
    std::condition_variable _cv;
    std::queue<std::string> _receivedMessages;
};

// ============================================================================
// Client TCP Simple pour les Tests
// ============================================================================

/**
 * @brief Client TCP simple pour les tests
 */
class TestTCPClient {
public:
    TestTCPClient()
        : _socket(_ioContext)
        , _connected(false)
    {}

    ~TestTCPClient() {
        disconnect();
    }

    bool connect(const std::string& host, std::uint16_t port, int timeoutMs = 1000) {
        try {
            tcp::resolver resolver(_ioContext);
            auto endpoints = resolver.resolve(host, std::to_string(port));

            boost::asio::connect(_socket, endpoints);
            _connected = true;

            // Démarrer le thread d'I/O pour les lectures asynchrones
            _ioThread = std::thread([this]() {
                try {
                    startRead();
                    _ioContext.run();
                } catch (...) {}
            });

            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    void disconnect() {
        _connected = false;
        _ioContext.stop();

        try {
            if (_socket.is_open()) {
                _socket.close();
            }
        } catch (...) {}

        if (_ioThread.joinable()) {
            _ioThread.join();
        }
    }

    bool send(const std::string& message) {
        if (!_connected) return false;

        try {
            boost::asio::write(_socket, boost::asio::buffer(message));
            return true;
        } catch (const std::exception&) {
            _connected = false;
            return false;
        }
    }

    std::string receive(int timeoutMs = 1000) {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() {
            return !_receivedMessages.empty();
        });

        if (_receivedMessages.empty()) return "";

        std::string msg = _receivedMessages.front();
        _receivedMessages.pop();
        return msg;
    }

    bool isConnected() const { return _connected; }

private:
    void startRead() {
        if (!_connected) return;

        _socket.async_read_some(
            boost::asio::buffer(_buffer, sizeof(_buffer)),
            [this](boost::system::error_code ec, std::size_t bytes) {
                if (!ec && bytes > 0) {
                    std::string received(_buffer, bytes);
                    {
                        std::lock_guard<std::mutex> lock(_mutex);
                        _receivedMessages.push(received);
                    }
                    _cv.notify_all();

                    if (_connected) {
                        startRead();
                    }
                } else {
                    _connected = false;
                }
            });
    }

    boost::asio::io_context _ioContext;
    tcp::socket _socket;
    std::thread _ioThread;
    std::atomic<bool> _connected;
    char _buffer[4096];

    std::mutex _mutex;
    std::condition_variable _cv;
    std::queue<std::string> _receivedMessages;
};

// ============================================================================
// Tests de Connexion
// ============================================================================

class TCPConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        _server = std::make_unique<TestEchoServer>(0);  // Port dynamique
        _server->start();
    }

    void TearDown() override {
        _server->stop();
    }

    std::unique_ptr<TestEchoServer> _server;
};

/**
 * @test Connexion TCP basique réussie
 */
TEST_F(TCPConnectionTest, BasicConnectionSuccess) {
    // Utiliser un port fixe pour ce test
    TestEchoServer server(19876);
    server.start();

    TestTCPClient client;
    bool connected = client.connect("127.0.0.1", 19876);

    EXPECT_TRUE(connected);
    EXPECT_TRUE(client.isConnected());

    client.disconnect();
    server.stop();
}

/**
 * @test Connexion à un port fermé échoue
 */
TEST_F(TCPConnectionTest, ConnectionToClosedPortFails) {
    TestTCPClient client;
    // Port qui ne devrait pas avoir de serveur
    bool connected = client.connect("127.0.0.1", 59999, 500);

    EXPECT_FALSE(connected);
    EXPECT_FALSE(client.isConnected());
}

/**
 * @test Déconnexion propre
 */
TEST_F(TCPConnectionTest, CleanDisconnect) {
    TestEchoServer server(19877);
    server.start();

    TestTCPClient client;
    ASSERT_TRUE(client.connect("127.0.0.1", 19877));
    EXPECT_TRUE(client.isConnected());

    client.disconnect();
    EXPECT_FALSE(client.isConnected());

    server.stop();
}

// ============================================================================
// Tests d'Envoi/Réception
// ============================================================================

class TCPCommunicationTest : public ::testing::Test {
protected:
    void SetUp() override {
        _server = std::make_unique<TestEchoServer>(19878);
        _server->start();

        _client = std::make_unique<TestTCPClient>();
        ASSERT_TRUE(_client->connect("127.0.0.1", 19878));
    }

    void TearDown() override {
        _client->disconnect();
        _server->stop();
    }

    std::unique_ptr<TestEchoServer> _server;
    std::unique_ptr<TestTCPClient> _client;
};

/**
 * @test Envoi et réception d'un message simple
 */
TEST_F(TCPCommunicationTest, SendReceiveSimpleMessage) {
    std::string message = "Hello, Server!";

    ASSERT_TRUE(_client->send(message));

    std::string response = _client->receive(1000);
    EXPECT_EQ(response, message);  // Echo server renvoie le même message
}

/**
 * @test Envoi de plusieurs messages
 */
TEST_F(TCPCommunicationTest, SendMultipleMessages) {
    std::vector<std::string> messages = {
        "Message 1",
        "Message 2",
        "Message 3"
    };

    for (const auto& msg : messages) {
        ASSERT_TRUE(_client->send(msg));
        std::string response = _client->receive(1000);
        EXPECT_EQ(response, msg);
    }
}

/**
 * @test Envoi de message avec caractères spéciaux
 */
TEST_F(TCPCommunicationTest, SendSpecialCharacters) {
    std::string message = "Special: @#$%^&*()!~`";

    ASSERT_TRUE(_client->send(message));

    std::string response = _client->receive(1000);
    EXPECT_EQ(response, message);
}

/**
 * @test Envoi de commande au format protocole
 */
TEST_F(TCPCommunicationTest, SendProtocolCommand) {
    std::string command = "LOGIN|username|password";

    ASSERT_TRUE(_client->send(command));

    std::string response = _client->receive(1000);
    EXPECT_EQ(response, command);
}

/**
 * @test Envoi de message long
 */
TEST_F(TCPCommunicationTest, SendLongMessage) {
    std::string message(2000, 'x');

    ASSERT_TRUE(_client->send(message));

    std::string response = _client->receive(2000);
    EXPECT_EQ(response, message);
}

/**
 * @test Envoi de données binaires (simulées)
 */
TEST_F(TCPCommunicationTest, SendBinaryData) {
    std::string binaryData;
    for (int i = 0; i < 256; i++) {
        binaryData += static_cast<char>(i);
    }

    ASSERT_TRUE(_client->send(binaryData));

    std::string response = _client->receive(1000);
    EXPECT_EQ(response.size(), binaryData.size());
}

// ============================================================================
// Tests de Robustesse
// ============================================================================

class TCPRobustnessTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @test Envoi après déconnexion échoue
 */
TEST_F(TCPRobustnessTest, SendAfterDisconnectFails) {
    TestEchoServer server(19879);
    server.start();

    TestTCPClient client;
    ASSERT_TRUE(client.connect("127.0.0.1", 19879));

    client.disconnect();

    bool sent = client.send("This should fail");
    EXPECT_FALSE(sent);

    server.stop();
}

/**
 * @test Multiple connexions séquentielles
 */
TEST_F(TCPRobustnessTest, MultipleSequentialConnections) {
    TestEchoServer server(19880);
    server.start();

    for (int i = 0; i < 3; i++) {
        TestTCPClient client;
        ASSERT_TRUE(client.connect("127.0.0.1", 19880));
        EXPECT_TRUE(client.isConnected());

        std::string msg = "Connection " + std::to_string(i);
        ASSERT_TRUE(client.send(msg));

        std::string response = client.receive(1000);
        EXPECT_EQ(response, msg);

        client.disconnect();
    }

    server.stop();
}

/**
 * @test Reconnexion après déconnexion
 */
TEST_F(TCPRobustnessTest, ReconnectAfterDisconnect) {
    TestEchoServer server(19881);
    server.start();

    TestTCPClient client1;
    ASSERT_TRUE(client1.connect("127.0.0.1", 19881));
    ASSERT_TRUE(client1.send("First connection"));
    client1.receive(500);
    client1.disconnect();

    // Attendre un peu pour que le serveur libère les ressources
    std::this_thread::sleep_for(100ms);

    TestTCPClient client2;
    ASSERT_TRUE(client2.connect("127.0.0.1", 19881));
    ASSERT_TRUE(client2.send("Second connection"));

    std::string response = client2.receive(1000);
    EXPECT_EQ(response, "Second connection");

    client2.disconnect();
    server.stop();
}

// ============================================================================
// Tests de Performance
// ============================================================================

class TCPPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        _server = std::make_unique<TestEchoServer>(19882);
        _server->start();
    }

    void TearDown() override {
        _server->stop();
    }

    std::unique_ptr<TestEchoServer> _server;
};

/**
 * @test Envoi rapide de nombreux petits messages
 */
TEST_F(TCPPerformanceTest, RapidSmallMessages) {
    TestTCPClient client;
    ASSERT_TRUE(client.connect("127.0.0.1", 19882));

    const int numMessages = 100;
    std::string message = "Quick message";

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numMessages; i++) {
        ASSERT_TRUE(client.send(message));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Devrait prendre moins de 5 secondes pour 100 messages
    EXPECT_LT(duration.count(), 5000);

    client.disconnect();
}

/**
 * @test Latence aller-retour
 */
TEST_F(TCPPerformanceTest, RoundTripLatency) {
    TestTCPClient client;
    ASSERT_TRUE(client.connect("127.0.0.1", 19882));

    std::string message = "Latency test";

    auto start = std::chrono::high_resolution_clock::now();
    ASSERT_TRUE(client.send(message));
    std::string response = client.receive(1000);
    auto end = std::chrono::high_resolution_clock::now();

    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_EQ(response, message);
    // La latence localhost devrait être < 100ms
    EXPECT_LT(latency.count(), 100);

    client.disconnect();
}

// ============================================================================
// Tests avec Protobuf sur TCP
// ============================================================================

#include "user.pb.h"
#include "auth.pb.h"
#include "game.pb.h"

class TCPProtobufTest : public ::testing::Test {
protected:
    void SetUp() override {
        _server = std::make_unique<TestEchoServer>(19883);
        _server->start();

        _client = std::make_unique<TestTCPClient>();
        ASSERT_TRUE(_client->connect("127.0.0.1", 19883));
    }

    void TearDown() override {
        _client->disconnect();
        _server->stop();
    }

    std::unique_ptr<TestEchoServer> _server;
    std::unique_ptr<TestTCPClient> _client;
};

/**
 * @test Envoi d'un message User sérialisé
 */
TEST_F(TCPProtobufTest, SendUserMessage) {
    rtype::User user;
    user.set_username("networkuser");
    user.set_password("networkpass");

    std::string serialized;
    ASSERT_TRUE(user.SerializeToString(&serialized));

    ASSERT_TRUE(_client->send(serialized));

    std::string response = _client->receive(1000);
    EXPECT_EQ(response, serialized);

    // Désérialiser la réponse
    rtype::User received;
    ASSERT_TRUE(received.ParseFromString(response));
    EXPECT_EQ(received.username(), "networkuser");
}

/**
 * @test Envoi d'un AuthMessage sérialisé
 */
TEST_F(TCPProtobufTest, SendAuthMessage) {
    rtype::auth::AuthMessage msg;
    msg.set_type(rtype::auth::AUTH_LOGIN_REQUEST);
    msg.set_sequence_id(1);

    auto* request = msg.mutable_login_request();
    request->set_username("testuser");
    request->set_password_hash("hash123");
    request->set_client_version("1.0.0");

    std::string serialized;
    ASSERT_TRUE(msg.SerializeToString(&serialized));

    ASSERT_TRUE(_client->send(serialized));

    std::string response = _client->receive(1000);

    rtype::auth::AuthMessage received;
    ASSERT_TRUE(received.ParseFromString(response));
    EXPECT_EQ(received.type(), rtype::auth::AUTH_LOGIN_REQUEST);
    EXPECT_EQ(received.login_request().username(), "testuser");
}

/**
 * @test Envoi d'un GameMessage avec WorldSnapshot
 */
TEST_F(TCPProtobufTest, SendGameMessage) {
    rtype::game::GameMessage msg;
    msg.set_type(rtype::game::MSG_WORLD_SNAPSHOT);
    msg.set_sequence_id(100);

    auto* snapshot = msg.mutable_world_snapshot();
    snapshot->set_server_tick(500);
    snapshot->set_state(rtype::game::GAME_RUNNING);

    // Ajouter quelques entités
    for (int i = 0; i < 5; i++) {
        auto* entity = snapshot->add_entities();
        entity->set_network_id(i);
        entity->set_type(rtype::game::ENTITY_PLAYER);
        entity->mutable_position()->set_x(static_cast<float>(i * 100));
        entity->mutable_position()->set_y(static_cast<float>(i * 50));
    }

    std::string serialized;
    ASSERT_TRUE(msg.SerializeToString(&serialized));

    ASSERT_TRUE(_client->send(serialized));

    std::string response = _client->receive(1000);

    rtype::game::GameMessage received;
    ASSERT_TRUE(received.ParseFromString(response));
    EXPECT_EQ(received.world_snapshot().entities_size(), 5);
}

/**
 * @test Envoi avec préfixe de longueur (framing)
 */
TEST_F(TCPProtobufTest, SendWithLengthPrefix) {
    rtype::User user;
    user.set_username("framed_user");
    user.set_password("framed_pass");

    std::string serialized;
    ASSERT_TRUE(user.SerializeToString(&serialized));

    // Ajouter un préfixe de longueur (4 bytes, little-endian)
    uint32_t length = static_cast<uint32_t>(serialized.size());
    std::string framed;
    framed.append(reinterpret_cast<char*>(&length), sizeof(length));
    framed.append(serialized);

    ASSERT_TRUE(_client->send(framed));

    std::string response = _client->receive(1000);
    EXPECT_EQ(response, framed);

    // Extraire et désérialiser
    if (response.size() >= sizeof(uint32_t)) {
        uint32_t receivedLength;
        std::memcpy(&receivedLength, response.data(), sizeof(uint32_t));
        EXPECT_EQ(receivedLength, serialized.size());

        std::string payload = response.substr(sizeof(uint32_t));
        rtype::User received;
        ASSERT_TRUE(received.ParseFromString(payload));
        EXPECT_EQ(received.username(), "framed_user");
    }
}

/**
 * @test Envoi de multiple messages Protobuf séquentiels
 */
TEST_F(TCPProtobufTest, SendMultipleProtobufMessages) {
    for (int i = 0; i < 10; i++) {
        rtype::game::ClientInput input;
        input.set_sequence_number(i);
        input.set_client_tick(i * 100);
        input.mutable_input()->set_shoot(i % 2 == 0);

        std::string serialized;
        ASSERT_TRUE(input.SerializeToString(&serialized));

        ASSERT_TRUE(_client->send(serialized));

        std::string response = _client->receive(500);

        rtype::game::ClientInput received;
        ASSERT_TRUE(received.ParseFromString(response));
        EXPECT_EQ(received.sequence_number(), static_cast<uint32_t>(i));
    }
}
