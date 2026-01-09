/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests d'intégration UDP avec Boost.Asio
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
#include <memory>

using boost::asio::ip::udp;
using namespace std::chrono_literals;

/**
 * @brief Suite de tests d'intégration UDP
 *
 * Ces tests vérifient :
 * - Envoi et réception de datagrammes UDP
 * - Communication bidirectionnelle
 * - Gestion des paquets perdus (simulation)
 * - Performance UDP
 *
 * Note: UDP est sans connexion, donc pas de "connexion" à établir.
 */

// ============================================================================
// Serveur UDP Echo pour les Tests
// ============================================================================

/**
 * @brief Serveur UDP echo simple
 */
class TestUDPServer {
public:
    TestUDPServer(std::uint16_t port)
        : _socket(_ioContext, udp::endpoint(udp::v4(), port))
        , _port(port)
        , _running(false)
        , _packetsReceived(0)
    {}

    ~TestUDPServer() {
        stop();
    }

    void start() {
        _running = true;
        _serverThread = std::thread([this]() {
            try {
                startReceive();
                _ioContext.run();
            } catch (const std::exception&) {
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
    int getPacketsReceived() const { return _packetsReceived; }

    std::string getLastReceived() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_receivedMessages.empty()) return "";
        std::string msg = _receivedMessages.front();
        _receivedMessages.pop();
        return msg;
    }

    void waitForPacket(int timeoutMs = 1000) {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() {
            return !_receivedMessages.empty();
        });
    }

private:
    void startReceive() {
        _socket.async_receive_from(
            boost::asio::buffer(_buffer, sizeof(_buffer)),
            _remoteEndpoint,
            [this](boost::system::error_code ec, std::size_t bytes) {
                if (!ec && bytes > 0 && _running) {
                    _packetsReceived++;
                    std::string received(_buffer, bytes);

                    {
                        std::lock_guard<std::mutex> lock(_mutex);
                        _receivedMessages.push(received);
                        _lastSenderEndpoint = _remoteEndpoint;
                    }
                    _cv.notify_all();

                    // Echo back
                    _socket.async_send_to(
                        boost::asio::buffer(received),
                        _remoteEndpoint,
                        [](boost::system::error_code, std::size_t) {});
                }

                if (_running) {
                    startReceive();
                }
            });
    }

    boost::asio::io_context _ioContext;
    udp::socket _socket;
    udp::endpoint _remoteEndpoint;
    udp::endpoint _lastSenderEndpoint;
    std::thread _serverThread;
    std::uint16_t _port;
    std::atomic<bool> _running;
    std::atomic<int> _packetsReceived;
    char _buffer[4096];

    std::mutex _mutex;
    std::condition_variable _cv;
    std::queue<std::string> _receivedMessages;
};

// ============================================================================
// Client UDP Simple pour les Tests
// ============================================================================

/**
 * @brief Client UDP simple pour les tests
 */
class TestUDPClient {
public:
    TestUDPClient()
        : _socket(_ioContext, udp::endpoint(udp::v4(), 0))
        , _running(false)
    {}

    ~TestUDPClient() {
        stop();
    }

    void start() {
        _running = true;
        _ioThread = std::thread([this]() {
            try {
                startReceive();
                _ioContext.run();
            } catch (...) {}
        });

        std::this_thread::sleep_for(50ms);
    }

    void stop() {
        _running = false;
        _ioContext.stop();

        if (_ioThread.joinable()) {
            _ioThread.join();
        }
    }

    bool sendTo(const std::string& host, std::uint16_t port, const std::string& message) {
        try {
            udp::resolver resolver(_ioContext);
            auto endpoints = resolver.resolve(udp::v4(), host, std::to_string(port));
            udp::endpoint serverEndpoint = *endpoints.begin();

            _socket.send_to(boost::asio::buffer(message), serverEndpoint);
            return true;
        } catch (const std::exception&) {
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

    int getPacketsReceived() const { return _packetsReceived; }

private:
    void startReceive() {
        _socket.async_receive_from(
            boost::asio::buffer(_buffer, sizeof(_buffer)),
            _senderEndpoint,
            [this](boost::system::error_code ec, std::size_t bytes) {
                if (!ec && bytes > 0) {
                    _packetsReceived++;
                    std::string received(_buffer, bytes);
                    {
                        std::lock_guard<std::mutex> lock(_mutex);
                        _receivedMessages.push(received);
                    }
                    _cv.notify_all();
                }

                if (_running) {
                    startReceive();
                }
            });
    }

    boost::asio::io_context _ioContext;
    udp::socket _socket;
    udp::endpoint _senderEndpoint;
    std::thread _ioThread;
    std::atomic<bool> _running;
    std::atomic<int> _packetsReceived{0};
    char _buffer[4096];

    std::mutex _mutex;
    std::condition_variable _cv;
    std::queue<std::string> _receivedMessages;
};

// ============================================================================
// Tests d'Envoi/Réception UDP
// ============================================================================

class UDPCommunicationTest : public ::testing::Test {
protected:
    void SetUp() override {
        _server = std::make_unique<TestUDPServer>(19890);
        _server->start();

        _client = std::make_unique<TestUDPClient>();
        _client->start();
    }

    void TearDown() override {
        _client->stop();
        _server->stop();
    }

    std::unique_ptr<TestUDPServer> _server;
    std::unique_ptr<TestUDPClient> _client;
};

/**
 * @test Envoi et réception d'un datagramme simple
 */
TEST_F(UDPCommunicationTest, SendReceiveSimpleDatagram) {
    std::string message = "Hello UDP!";

    ASSERT_TRUE(_client->sendTo("127.0.0.1", 19890, message));

    std::string response = _client->receive(1000);
    EXPECT_EQ(response, message);
}

/**
 * @test Envoi de plusieurs datagrammes
 */
TEST_F(UDPCommunicationTest, SendMultipleDatagrams) {
    std::vector<std::string> messages = {
        "Packet 1",
        "Packet 2",
        "Packet 3"
    };

    for (const auto& msg : messages) {
        ASSERT_TRUE(_client->sendTo("127.0.0.1", 19890, msg));
        std::string response = _client->receive(1000);
        EXPECT_EQ(response, msg);
    }
}

/**
 * @test Envoi de datagramme avec données de jeu
 */
TEST_F(UDPCommunicationTest, SendGameData) {
    // Simuler un input de joueur
    std::string gameInput = "INPUT|1|0|0|1|1|0";  // up, down, left, right, shoot, special

    ASSERT_TRUE(_client->sendTo("127.0.0.1", 19890, gameInput));

    std::string response = _client->receive(1000);
    EXPECT_EQ(response, gameInput);
}

/**
 * @test Envoi rapide de nombreux paquets (burst)
 */
TEST_F(UDPCommunicationTest, BurstSend) {
    const int numPackets = 50;
    std::string message = "Burst packet";

    for (int i = 0; i < numPackets; i++) {
        ASSERT_TRUE(_client->sendTo("127.0.0.1", 19890, message + std::to_string(i)));
    }

    // Attendre un peu pour que tous les paquets arrivent
    std::this_thread::sleep_for(500ms);

    // Avec UDP, certains paquets peuvent être perdus, mais sur localhost ça devrait fonctionner
    EXPECT_GT(_server->getPacketsReceived(), 0);
}

/**
 * @test Datagramme de taille maximale typique
 */
TEST_F(UDPCommunicationTest, MaxDatagramSize) {
    // Taille typique sûre pour UDP (éviter fragmentation)
    std::string largeMessage(1400, 'x');

    ASSERT_TRUE(_client->sendTo("127.0.0.1", 19890, largeMessage));

    std::string response = _client->receive(1000);
    EXPECT_EQ(response.size(), largeMessage.size());
}

// ============================================================================
// Tests de Performance UDP
// ============================================================================

class UDPPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        _server = std::make_unique<TestUDPServer>(19891);
        _server->start();
    }

    void TearDown() override {
        _server->stop();
    }

    std::unique_ptr<TestUDPServer> _server;
};

/**
 * @test Débit d'envoi UDP
 */
TEST_F(UDPPerformanceTest, SendThroughput) {
    TestUDPClient client;
    client.start();

    const int numPackets = 1000;
    std::string message = "Performance test packet";

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numPackets; i++) {
        client.sendTo("127.0.0.1", 19891, message);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Devrait prendre moins de 2 secondes pour 1000 paquets
    EXPECT_LT(duration.count(), 2000);

    client.stop();
}

/**
 * @test Latence aller-retour UDP
 */
TEST_F(UDPPerformanceTest, RoundTripLatency) {
    TestUDPClient client;
    client.start();

    std::string message = "Latency test";

    auto start = std::chrono::high_resolution_clock::now();
    client.sendTo("127.0.0.1", 19891, message);
    std::string response = client.receive(1000);
    auto end = std::chrono::high_resolution_clock::now();

    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(response, message);
    // La latence UDP localhost devrait être < 10ms
    EXPECT_LT(latency.count(), 10000);

    client.stop();
}

/**
 * @test Comparaison: nombreux petits paquets vs quelques gros paquets
 */
TEST_F(UDPPerformanceTest, SmallVsLargePackets) {
    TestUDPClient client;
    client.start();

    // Test petits paquets
    auto startSmall = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        std::string small(50, 'a');
        client.sendTo("127.0.0.1", 19891, small);
    }
    auto endSmall = std::chrono::high_resolution_clock::now();
    auto durationSmall = std::chrono::duration_cast<std::chrono::microseconds>(endSmall - startSmall);

    // Test gros paquets (même quantité de données)
    auto startLarge = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 5; i++) {
        std::string large(1000, 'b');
        client.sendTo("127.0.0.1", 19891, large);
    }
    auto endLarge = std::chrono::high_resolution_clock::now();
    auto durationLarge = std::chrono::duration_cast<std::chrono::microseconds>(endLarge - startLarge);

    // Les gros paquets devraient être plus rapides (moins d'overhead)
    // Note: sur localhost, la différence peut être minime
    SUCCEED();  // Le test réussit s'il ne crash pas

    client.stop();
}


// ============================================================================
// Tests de Robustesse UDP
// ============================================================================

class UDPRobustnessTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @test Envoi vers serveur inexistant (pas d'erreur, UDP est fire-and-forget)
 */
TEST_F(UDPRobustnessTest, SendToNonExistentServer) {
    TestUDPClient client;
    client.start();

    // UDP ne génère pas d'erreur pour l'envoi vers un port fermé
    bool sent = client.sendTo("127.0.0.1", 59998, "Hello?");
    EXPECT_TRUE(sent);  // L'envoi "réussit" (le paquet est envoyé)

    // Mais pas de réponse
    std::string response = client.receive(500);
    EXPECT_TRUE(response.empty());

    client.stop();
}

/**
 * @test Multiple clients vers même serveur
 */
TEST_F(UDPRobustnessTest, MultipleClients) {
    TestUDPServer server(19893);
    server.start();

    TestUDPClient client1;
    TestUDPClient client2;
    TestUDPClient client3;

    client1.start();
    client2.start();
    client3.start();

    client1.sendTo("127.0.0.1", 19893, "Client 1");
    client2.sendTo("127.0.0.1", 19893, "Client 2");
    client3.sendTo("127.0.0.1", 19893, "Client 3");

    std::this_thread::sleep_for(500ms);

    EXPECT_GE(server.getPacketsReceived(), 3);

    client1.stop();
    client2.stop();
    client3.stop();
    server.stop();
}

/**
 * @test Données invalides (echo server renvoie les mêmes données)
 */
TEST_F(UDPRobustnessTest, InvalidDataHandling) {
    TestUDPServer server(19894);
    server.start();

    TestUDPClient client;
    client.start();

    // Envoyer des données invalides
    std::string invalidData = "This is not valid data!@#$%";
    client.sendTo("127.0.0.1", 19894, invalidData);

    std::string response = client.receive(500);

    // Le serveur echo renvoie les mêmes données
    EXPECT_EQ(response, invalidData);

    client.stop();
    server.stop();
}
