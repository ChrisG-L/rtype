---
tags:
  - developpement
  - tests
---

# Guide de Test

Stratégie et outils de test pour R-Type.

## Framework

Nous utilisons **GoogleTest** (gtest) pour les tests unitaires.

```cmake
# CMakeLists.txt
find_package(GTest REQUIRED)

add_executable(rtype_tests
    tests/test_main.cpp
    tests/test_gameworld.cpp
    tests/test_protocol.cpp
)

target_link_libraries(rtype_tests
    PRIVATE
        rtype_common
        GTest::gtest
        GTest::gtest_main
)
```

---

## Structure des Tests

```
tests/
├── unit/
│   ├── server/
│   │   ├── test_gameworld.cpp
│   │   ├── test_roommanager.cpp
│   │   └── test_wavemanager.cpp
│   ├── client/
│   │   ├── test_prediction.cpp
│   │   └── test_interpolation.cpp
│   └── common/
│       ├── test_protocol.cpp
│       ├── test_serializer.cpp
│       └── test_packet.cpp
├── integration/
│   ├── test_network.cpp
│   └── test_game_session.cpp
└── test_main.cpp
```

---

## Tests Unitaires

### Exemple: GameWorld

```cpp
#include <gtest/gtest.h>
#include "server/GameWorld.hpp"

class GameWorldTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<GameWorld>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<GameWorld> world_;
};

TEST_F(GameWorldTest, AddPlayer) {
    world_->addPlayer(1, {100.0f, 100.0f});

    auto* player = world_->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->id(), 1);
    EXPECT_FLOAT_EQ(player->x(), 100.0f);
    EXPECT_FLOAT_EQ(player->y(), 100.0f);
}

TEST_F(GameWorldTest, RemovePlayer) {
    world_->addPlayer(1, {100.0f, 100.0f});
    world_->removePlayer(1);

    EXPECT_EQ(world_->getPlayer(1), nullptr);
}

TEST_F(GameWorldTest, ProcessInput) {
    world_->addPlayer(1, {100.0f, 100.0f});

    Input input{.sequence = 1, .keys = KEY_RIGHT};
    world_->processInput(1, input);

    auto* player = world_->getPlayer(1);
    EXPECT_GT(player->x(), 100.0f);  // Moved right
}

TEST_F(GameWorldTest, CollisionDetection) {
    world_->addPlayer(1, {100.0f, 100.0f});
    world_->spawnEnemy(EnemyType::Basic, {100.0f, 100.0f});

    world_->tick();

    auto* player = world_->getPlayer(1);
    EXPECT_LT(player->health(), Player::MAX_HEALTH);
}
```

### Exemple: Serializer

```cpp
#include <gtest/gtest.h>
#include "network/Serializer.hpp"

TEST(SerializerTest, WriteReadInt) {
    Serializer writer;
    writer.write<uint32_t>(42);

    Serializer reader(writer.data());
    EXPECT_EQ(reader.read<uint32_t>(), 42);
}

TEST(SerializerTest, WriteReadFloat) {
    Serializer writer;
    writer.write<float>(3.14f);

    Serializer reader(writer.data());
    EXPECT_FLOAT_EQ(reader.read<float>(), 3.14f);
}

TEST(SerializerTest, WriteReadString) {
    Serializer writer;
    writer.writeString("Hello World");

    Serializer reader(writer.data());
    EXPECT_EQ(reader.readString(), "Hello World");
}

TEST(SerializerTest, BufferUnderflow) {
    Serializer reader({0x01, 0x02});  // Only 2 bytes

    EXPECT_THROW(reader.read<uint32_t>(), std::runtime_error);
}
```

### Exemple: Packet

```cpp
#include <gtest/gtest.h>
#include "network/Packet.hpp"

TEST(PacketTest, SerializeDeserialize) {
    LoginPacket login;
    std::strcpy(login.username, "player1");
    std::strcpy(login.passwordHash, "hash123");

    Packet packet(PacketType::Login);
    packet.write(login);

    auto bytes = packet.serialize();

    Packet received;
    ASSERT_TRUE(received.deserialize(bytes));
    EXPECT_EQ(received.type(), PacketType::Login);

    auto data = received.as<LoginPacket>();
    EXPECT_STREQ(data.username, "player1");
}

TEST(PacketTest, InvalidMagic) {
    std::vector<uint8_t> invalid = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};

    Packet packet;
    EXPECT_FALSE(packet.deserialize(invalid));
}
```

---

## Tests d'Intégration

### Exemple: Session de Jeu

```cpp
#include <gtest/gtest.h>
#include "server/Application.hpp"
#include "client/NetworkClient.hpp"

class GameSessionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start server in background
        server_ = std::make_unique<TestServer>();
        server_->start(4242);

        // Connect client
        client_ = std::make_unique<NetworkClient>();
        client_->connect("localhost", 4242);
    }

    void TearDown() override {
        client_->disconnect();
        server_->stop();
    }

    std::unique_ptr<TestServer> server_;
    std::unique_ptr<NetworkClient> client_;
};

TEST_F(GameSessionTest, LoginFlow) {
    // Send login
    client_->send(PacketFactory::createLogin("player1", "hash"));

    // Wait for response
    auto packets = waitForPackets(client_.get(), 1);
    ASSERT_EQ(packets.size(), 1);
    EXPECT_EQ(packets[0].type(), PacketType::LoginAck);

    auto ack = packets[0].as<LoginAckPacket>();
    EXPECT_TRUE(ack.success);
}

TEST_F(GameSessionTest, JoinRoom) {
    // Login first
    login(client_.get(), "player1");

    // Create and join room
    client_->send(PacketFactory::createRoom("Test Room"));
    auto packets = waitForPackets(client_.get(), 1);

    auto ack = packets[0].as<CreateRoomAckPacket>();
    EXPECT_TRUE(ack.success);
    EXPECT_GT(ack.roomId, 0);
}
```

---

## Mocks

### Mock Repository

```cpp
class MockPlayerRepository : public IPlayerRepository {
public:
    MOCK_METHOD(std::optional<Player>, findById,
                (const std::string&), (override));
    MOCK_METHOD(std::optional<Player>, findByUsername,
                (const std::string&), (override));
    MOCK_METHOD(void, save, (const Player&), (override));
    MOCK_METHOD(void, update, (const Player&), (override));
    MOCK_METHOD(void, remove, (const std::string&), (override));
};

TEST(AuthServiceTest, LoginSuccess) {
    MockPlayerRepository repo;
    AuthService auth(repo);

    Player existingPlayer{.id = "1", .username = "player1"};

    EXPECT_CALL(repo, findByUsername("player1"))
        .WillOnce(Return(existingPlayer));

    auto result = auth.login("player1", "correctPassword");
    EXPECT_TRUE(result.success);
}
```

---

## Exécution

```bash
# Tous les tests
ctest --preset conan-release

# Tests spécifiques
./build/tests/rtype_tests --gtest_filter="GameWorldTest.*"

# Avec output détaillé
./build/tests/rtype_tests --gtest_output=xml:report.xml

# Coverage (avec gcov)
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make
./rtype_tests
gcov *.gcda
```

---

## Coverage

### Configuration CMake

```cmake
option(ENABLE_COVERAGE "Enable coverage" OFF)

if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()
```

### Générer le Rapport

```bash
# Avec lcov
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

---

## CI Integration

```yaml
# .github/workflows/tests.yml
test:
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v4

    - name: Build
      run: |
        cmake --preset conan-release
        cmake --build --preset conan-release

    - name: Test
      run: ctest --preset conan-release --output-on-failure

    - name: Upload Results
      uses: actions/upload-artifact@v3
      with:
        name: test-results
        path: build/Testing/
```
