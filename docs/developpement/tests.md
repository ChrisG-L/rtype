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
find_package(GTest CONFIG REQUIRED)

add_executable(rtype_server_tests
    tests/server/main.cpp
    tests/server/domain/entities/PlayerTest.cpp
    tests/server/domain/value_objects/HealthTest.cpp
    # ...
)

target_link_libraries(rtype_server_tests
    PRIVATE
        rtype_server_lib
        GTest::gtest
        GTest::gtest_main
)
```

---

## Structure des Tests

```
tests/
├── server/
│   ├── main.cpp
│   ├── domain/
│   │   ├── entities/
│   │   │   └── PlayerTest.cpp
│   │   └── value_objects/
│   │       ├── EmailTest.cpp
│   │       ├── HealthTest.cpp
│   │       ├── PositionTest.cpp
│   │       └── UsernameTest.cpp
│   ├── network/
│   │   ├── ProtobufTest.cpp
│   │   ├── UDPIntegrationTest.cpp
│   │   └── VoiceProtocolTest.cpp
│   └── infrastructure/
│       └── session/
│           └── SessionManagerCryptoTest.cpp
│
├── client/
│   ├── main.cpp
│   ├── config/
│   │   └── ServerConfigManagerTest.cpp
│   ├── ui/
│   │   └── ServerConfigPanelTest.cpp
│   ├── utils/
│   │   ├── SignalTest.cpp
│   │   └── VecsTest.cpp
│   ├── audio/
│   │   ├── AudioDevicePersistenceTest.cpp
│   │   ├── PortAudioTest.cpp
│   │   ├── VoiceAudioTest.cpp
│   │   └── VoiceIntegrationTest.cpp
│   └── accessibility/
│       └── AccessibilityConfigTest.cpp
```

---

## Tests Unitaires

### Exemple: Player Entity

```cpp
#include <gtest/gtest.h>
#include "domain/entities/Player.hpp"
#include "domain/value_objects/Health.hpp"
#include "domain/value_objects/Position.hpp"
#include "domain/value_objects/player/PlayerId.hpp"
#include "domain/exceptions/HealthException.hpp"
#include "domain/exceptions/PositionException.hpp"

using namespace domain::entities;
using namespace domain::value_objects;
using namespace domain::value_objects::player;
using namespace domain::exceptions;

class PlayerTest : public ::testing::Test {
protected:
    // ID MongoDB valide pour les tests
    const std::string validId = "507f1f77bcf86cd799439011";

    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(PlayerTest, CreateWithAllParameters) {
    Health health(3.0f);
    PlayerId id(validId);
    Position position(100.0f, 200.0f, 50.0f);

    ASSERT_NO_THROW({
        Player player(health, id, position);
    });
}

TEST_F(PlayerTest, MoveChangesPosition) {
    Health health(3.0f);
    PlayerId id(validId);
    Position position(100.0f, 100.0f, 100.0f);

    Player player(health, id, position);
    player.move(50.0f, 25.0f, 10.0f);

    EXPECT_FLOAT_EQ(player.getPosition().getX(), 150.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getY(), 125.0f);
    EXPECT_FLOAT_EQ(player.getPosition().getZ(), 110.0f);
}

TEST_F(PlayerTest, MoveOutOfBoundsThrows) {
    Health health(3.0f);
    PlayerId id(validId);
    Position position(900.0f, 0.0f, 0.0f);

    Player player(health, id, position);

    EXPECT_THROW({
        player.move(200.0f, 0.0f, 0.0f);  // Dépasse limite 1000
    }, PositionException);
}

TEST_F(PlayerTest, HealAboveMaxThrows) {
    Health health(4.0f);
    PlayerId id(validId);

    Player player(health, id);

    EXPECT_THROW({
        player.heal(2.0f);  // 4.0 + 2.0 = 6.0 > 5.0 max
    }, HealthException);
}
```

### Exemple: Value Objects

```cpp
#include <gtest/gtest.h>
#include "domain/value_objects/Health.hpp"
#include "domain/exceptions/HealthException.hpp"

using namespace domain::value_objects;
using namespace domain::exceptions;

TEST(HealthTest, CreateValidHealth) {
    ASSERT_NO_THROW({
        Health health(3.0f);
    });
}

TEST(HealthTest, CreateInvalidHealthThrows) {
    EXPECT_THROW({
        Health health(6.0f);  // > 5.0 max
    }, HealthException);

    EXPECT_THROW({
        Health health(-1.0f);  // < 0 min
    }, HealthException);
}

TEST(HealthTest, BoundaryValues) {
    ASSERT_NO_THROW({ Health health(0.0f); });  // Min
    ASSERT_NO_THROW({ Health health(5.0f); });  // Max
}
```

### Exemple: Protocol Serialization

```cpp
#include <gtest/gtest.h>
#include "Protocol.hpp"

using namespace protocol;

TEST(ProtocolTest, PlayerStateRoundTrip) {
    PlayerState original;
    original.id = 1;
    original.x = 500;
    original.y = 300;
    original.health = 100;
    original.alive = 1;

    uint8_t buffer[PlayerState::WIRE_SIZE];
    original.to_bytes(buffer);

    auto parsed = PlayerState::from_bytes(buffer, PlayerState::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());

    EXPECT_EQ(parsed->id, original.id);
    EXPECT_EQ(parsed->x, original.x);
    EXPECT_EQ(parsed->y, original.y);
    EXPECT_EQ(parsed->health, original.health);
    EXPECT_EQ(parsed->alive, original.alive);
}

TEST(ProtocolTest, UDPHeaderSerialization) {
    UDPHeader header;
    header.type = static_cast<uint16_t>(MessageType::PlayerInput);
    header.sequence_num = 42;
    header.timestamp = 1234567890;

    uint8_t buffer[UDPHeader::WIRE_SIZE];
    header.to_bytes(buffer);

    auto parsed = UDPHeader::from_bytes(buffer, UDPHeader::WIRE_SIZE);
    ASSERT_TRUE(parsed.has_value());

    EXPECT_EQ(parsed->type, header.type);
    EXPECT_EQ(parsed->sequence_num, header.sequence_num);
    EXPECT_EQ(parsed->timestamp, header.timestamp);
}

TEST(ProtocolTest, BufferTooSmall) {
    uint8_t smallBuffer[3];  // Trop petit

    auto result = PlayerState::from_bytes(smallBuffer, 3);
    EXPECT_FALSE(result.has_value());
}
```

### Exemple: Voice Audio

```cpp
#include <gtest/gtest.h>
#include "audio/VoiceChatManager.hpp"

class VoiceAudioTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup
    }

    void TearDown() override {
        // Cleanup
    }
};

TEST_F(VoiceAudioTest, InitializeVoiceManager) {
    auto& voice = VoiceChatManager::getInstance();
    ASSERT_NO_THROW({
        voice.init();
    });
}

TEST_F(VoiceAudioTest, ListAudioDevices) {
    auto& voice = VoiceChatManager::getInstance();
    voice.init();

    auto inputs = voice.getInputDevices();
    auto outputs = voice.getOutputDevices();

    // Au moins un périphérique par défaut
    EXPECT_GE(inputs.size(), 0);
    EXPECT_GE(outputs.size(), 0);
}
```

---

## Exécution

```bash
# Tous les tests
./scripts/test.sh

# Via CTest
cd buildLinux
ctest --output-on-failure

# Tests spécifiques
./artifacts/server/linux/rtype_server_tests --gtest_filter="PlayerTest.*"

# Avec output détaillé
./artifacts/server/linux/rtype_server_tests --gtest_output=xml:report.xml

# Tests client
./artifacts/client/linux/rtype_client_tests --gtest_filter="VoiceAudioTest.*"
```

---

## Structure de Test Recommandée

### Fixture Pattern

```cpp
class GameWorldTest : public ::testing::Test {
protected:
    boost::asio::io_context _io_ctx;
    std::unique_ptr<infrastructure::game::GameWorld> _world;

    void SetUp() override {
        _world = std::make_unique<infrastructure::game::GameWorld>(_io_ctx);
    }

    void TearDown() override {
        _world.reset();
    }
};

TEST_F(GameWorldTest, AddPlayer) {
    udp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), 12345);
    auto playerId = _world->addPlayer(endpoint);

    ASSERT_TRUE(playerId.has_value());
    EXPECT_LT(*playerId, 4);  // MAX_PLAYERS = 4
}
```

### Test des Exceptions

```cpp
TEST(HealthTest, DamageExceedingHealthThrows) {
    Health health(1.0f);

    EXPECT_THROW({
        health.decrease(2.0f);  // 1.0 - 2.0 = -1.0 < 0
    }, HealthException);
}
```

### Test des Optionnels

```cpp
TEST(ProtocolTest, InvalidDataReturnsNullopt) {
    uint8_t garbage[] = {0xFF, 0xFF, 0xFF};

    auto result = PlayerState::from_bytes(garbage, sizeof(garbage));

    EXPECT_FALSE(result.has_value());
}
```

---

## Coverage

### Script de Coverage

```bash
# Avec gcov/lcov
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build
cd build && ctest

# Générer rapport
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' 'third_party/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

---

## CI Integration

Les tests sont exécutés automatiquement par Jenkins à chaque push.

```yaml
# Le workflow jenkins-trigger.yml déclenche le build
# Jenkins exécute les tests via scripts/test.sh
```

Voir [CI/CD](ci-cd.md) pour plus de détails.

---

## Best Practices

1. **Un test = un comportement**
2. **Noms descriptifs** : `TEST_F(PlayerTest, MoveOutOfBoundsThrows)`
3. **Arrange-Act-Assert** pattern
4. **Tester les cas limites** (0, max, boundary)
5. **Tester les erreurs** (exceptions, nullopt)
6. **Tests indépendants** (pas de dépendances entre tests)
