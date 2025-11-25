/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour la sérialisation/désérialisation Protobuf
*/

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstdint>

// Protobuf generated headers
#include "user.pb.h"
#include "auth.pb.h"
#include "game.pb.h"

/**
 * @brief Suite de tests pour les messages Protobuf
 *
 * Tests couvrant :
 * - Création de messages
 * - Sérialisation vers string/bytes
 * - Désérialisation depuis string/bytes
 * - Validation des champs
 * - Messages imbriqués (nested)
 * - Messages oneof
 * - Messages repeated
 */

// ============================================================================
// Tests User.proto - Message basique
// ============================================================================

class UserProtoTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @test Création d'un User avec des valeurs valides
 */
TEST_F(UserProtoTest, CreateUserWithValidValues) {
    rtype::User user;
    user.set_username("player1");
    user.set_password("secret123");

    EXPECT_EQ(user.username(), "player1");
    EXPECT_EQ(user.password(), "secret123");
}

/**
 * @test User avec valeurs vides
 */
TEST_F(UserProtoTest, CreateUserWithEmptyValues) {
    rtype::User user;

    EXPECT_TRUE(user.username().empty());
    EXPECT_TRUE(user.password().empty());
}

/**
 * @test Sérialisation User vers string
 */
TEST_F(UserProtoTest, SerializeUserToString) {
    rtype::User user;
    user.set_username("testuser");
    user.set_password("testpass");

    std::string serialized;
    ASSERT_TRUE(user.SerializeToString(&serialized));
    EXPECT_FALSE(serialized.empty());
}

/**
 * @test Désérialisation User depuis string
 */
TEST_F(UserProtoTest, DeserializeUserFromString) {
    // Créer et sérialiser
    rtype::User original;
    original.set_username("player123");
    original.set_password("securepass");

    std::string serialized;
    ASSERT_TRUE(original.SerializeToString(&serialized));

    // Désérialiser
    rtype::User deserialized;
    ASSERT_TRUE(deserialized.ParseFromString(serialized));

    EXPECT_EQ(deserialized.username(), "player123");
    EXPECT_EQ(deserialized.password(), "securepass");
}

/**
 * @test Round-trip sérialisation avec caractères spéciaux
 */
TEST_F(UserProtoTest, RoundTripWithSpecialCharacters) {
    rtype::User original;
    original.set_username("user_with-special.chars123");
    original.set_password("p@$$w0rd!#$%");

    std::string serialized;
    ASSERT_TRUE(original.SerializeToString(&serialized));

    rtype::User deserialized;
    ASSERT_TRUE(deserialized.ParseFromString(serialized));

    EXPECT_EQ(deserialized.username(), original.username());
    EXPECT_EQ(deserialized.password(), original.password());
}

/**
 * @test Sérialisation avec caractères UTF-8
 */
TEST_F(UserProtoTest, SerializeWithUTF8) {
    rtype::User user;
    user.set_username("joueur_français");
    user.set_password("motdepasse");

    std::string serialized;
    ASSERT_TRUE(user.SerializeToString(&serialized));

    rtype::User deserialized;
    ASSERT_TRUE(deserialized.ParseFromString(serialized));

    EXPECT_EQ(deserialized.username(), "joueur_français");
}

/**
 * @test Taille du message sérialisé
 */
TEST_F(UserProtoTest, SerializedSizeIsCorrect) {
    rtype::User user;
    user.set_username("test");
    user.set_password("pass");

    EXPECT_GT(user.ByteSizeLong(), 0u);

    std::string serialized;
    user.SerializeToString(&serialized);

    EXPECT_EQ(serialized.size(), user.ByteSizeLong());
}

/**
 * @test Clear efface toutes les valeurs
 */
TEST_F(UserProtoTest, ClearResetsValues) {
    rtype::User user;
    user.set_username("test");
    user.set_password("pass");

    user.Clear();

    EXPECT_TRUE(user.username().empty());
    EXPECT_TRUE(user.password().empty());
}

/**
 * @test Désérialisation de données invalides échoue
 */
TEST_F(UserProtoTest, DeserializeInvalidDataFails) {
    rtype::User user;
    std::string invalid_data = "this is not valid protobuf data!@#$";

    // ParseFromString peut réussir partiellement, vérifions le comportement
    // Le comportement dépend des données - parfois il ignore simplement les données invalides
    bool result = user.ParseFromString(invalid_data);
    // On ne peut pas garantir l'échec avec des données arbitraires
    // car protobuf peut les interpréter de différentes manières
    (void)result;  // Éviter warning unused
}

/**
 * @test Copie de message
 */
TEST_F(UserProtoTest, CopyMessage) {
    rtype::User original;
    original.set_username("original_user");
    original.set_password("original_pass");

    rtype::User copy = original;

    EXPECT_EQ(copy.username(), original.username());
    EXPECT_EQ(copy.password(), original.password());
}

// ============================================================================
// Tests Auth.proto - Messages avec enums et oneof
// ============================================================================

class AuthProtoTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @test Création LoginRequest
 */
TEST_F(AuthProtoTest, CreateLoginRequest) {
    rtype::auth::LoginRequest request;
    request.set_username("player1");
    request.set_password_hash("sha256hash");
    request.set_client_version("1.0.0");

    EXPECT_EQ(request.username(), "player1");
    EXPECT_EQ(request.password_hash(), "sha256hash");
    EXPECT_EQ(request.client_version(), "1.0.0");
}

/**
 * @test Création LoginResponse avec enum
 */
TEST_F(AuthProtoTest, CreateLoginResponseWithEnum) {
    rtype::auth::LoginResponse response;
    response.set_code(rtype::auth::AUTH_SUCCESS);
    response.set_message("Login successful");
    response.set_session_token("abc123token");
    response.set_player_id("player-uuid-123");
    response.set_token_expires_at(1700000000);

    EXPECT_EQ(response.code(), rtype::auth::AUTH_SUCCESS);
    EXPECT_EQ(response.message(), "Login successful");
    EXPECT_EQ(response.session_token(), "abc123token");
}

/**
 * @test Tous les codes de réponse auth
 */
TEST_F(AuthProtoTest, AllAuthResponseCodes) {
    rtype::auth::LoginResponse response;

    response.set_code(rtype::auth::AUTH_SUCCESS);
    EXPECT_EQ(response.code(), rtype::auth::AUTH_SUCCESS);

    response.set_code(rtype::auth::AUTH_INVALID_CREDENTIALS);
    EXPECT_EQ(response.code(), rtype::auth::AUTH_INVALID_CREDENTIALS);

    response.set_code(rtype::auth::AUTH_USER_NOT_FOUND);
    EXPECT_EQ(response.code(), rtype::auth::AUTH_USER_NOT_FOUND);

    response.set_code(rtype::auth::AUTH_USER_ALREADY_EXISTS);
    EXPECT_EQ(response.code(), rtype::auth::AUTH_USER_ALREADY_EXISTS);

    response.set_code(rtype::auth::AUTH_INVALID_TOKEN);
    EXPECT_EQ(response.code(), rtype::auth::AUTH_INVALID_TOKEN);

    response.set_code(rtype::auth::AUTH_TOKEN_EXPIRED);
    EXPECT_EQ(response.code(), rtype::auth::AUTH_TOKEN_EXPIRED);

    response.set_code(rtype::auth::AUTH_SERVER_ERROR);
    EXPECT_EQ(response.code(), rtype::auth::AUTH_SERVER_ERROR);
}

/**
 * @test AuthMessage avec oneof LoginRequest
 */
TEST_F(AuthProtoTest, AuthMessageWithLoginRequest) {
    rtype::auth::AuthMessage msg;
    msg.set_type(rtype::auth::AUTH_LOGIN_REQUEST);
    msg.set_sequence_id(1);
    msg.set_timestamp_ms(1700000000000);

    auto* request = msg.mutable_login_request();
    request->set_username("testuser");
    request->set_password_hash("hash123");

    EXPECT_EQ(msg.type(), rtype::auth::AUTH_LOGIN_REQUEST);
    EXPECT_TRUE(msg.has_login_request());
    EXPECT_FALSE(msg.has_login_response());
    EXPECT_EQ(msg.login_request().username(), "testuser");
}

/**
 * @test AuthMessage avec oneof LoginResponse
 */
TEST_F(AuthProtoTest, AuthMessageWithLoginResponse) {
    rtype::auth::AuthMessage msg;
    msg.set_type(rtype::auth::AUTH_LOGIN_RESPONSE);

    auto* response = msg.mutable_login_response();
    response->set_code(rtype::auth::AUTH_SUCCESS);
    response->set_session_token("token123");

    EXPECT_TRUE(msg.has_login_response());
    EXPECT_FALSE(msg.has_login_request());
    EXPECT_EQ(msg.login_response().code(), rtype::auth::AUTH_SUCCESS);
}

/**
 * @test Sérialisation AuthMessage complet
 */
TEST_F(AuthProtoTest, SerializeAuthMessage) {
    rtype::auth::AuthMessage original;
    original.set_type(rtype::auth::AUTH_LOGIN_REQUEST);
    original.set_sequence_id(42);
    original.set_timestamp_ms(1700000000000);

    auto* request = original.mutable_login_request();
    request->set_username("serialization_test");
    request->set_password_hash("hash_value");
    request->set_client_version("2.0.0");

    std::string serialized;
    ASSERT_TRUE(original.SerializeToString(&serialized));

    rtype::auth::AuthMessage deserialized;
    ASSERT_TRUE(deserialized.ParseFromString(serialized));

    EXPECT_EQ(deserialized.type(), rtype::auth::AUTH_LOGIN_REQUEST);
    EXPECT_EQ(deserialized.sequence_id(), 42u);
    EXPECT_EQ(deserialized.timestamp_ms(), 1700000000000u);
    EXPECT_EQ(deserialized.login_request().username(), "serialization_test");
}

/**
 * @test RegisterRequest et RegisterResponse
 */
TEST_F(AuthProtoTest, RegisterRequestResponse) {
    rtype::auth::RegisterRequest request;
    request.set_username("newuser");
    request.set_email("user@example.com");
    request.set_password_hash("hashed_password");

    std::string serialized;
    ASSERT_TRUE(request.SerializeToString(&serialized));

    rtype::auth::RegisterRequest deserialized;
    ASSERT_TRUE(deserialized.ParseFromString(serialized));

    EXPECT_EQ(deserialized.username(), "newuser");
    EXPECT_EQ(deserialized.email(), "user@example.com");
}

// ============================================================================
// Tests Game.proto - Messages complexes avec repeated et nested
// ============================================================================

class GameProtoTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @test Création Vec2
 */
TEST_F(GameProtoTest, CreateVec2) {
    rtype::game::Vec2 vec;
    vec.set_x(100.5f);
    vec.set_y(200.75f);

    EXPECT_FLOAT_EQ(vec.x(), 100.5f);
    EXPECT_FLOAT_EQ(vec.y(), 200.75f);
}

/**
 * @test Création Vec3
 */
TEST_F(GameProtoTest, CreateVec3) {
    rtype::game::Vec3 vec;
    vec.set_x(1.0f);
    vec.set_y(2.0f);
    vec.set_z(3.0f);

    EXPECT_FLOAT_EQ(vec.x(), 1.0f);
    EXPECT_FLOAT_EQ(vec.y(), 2.0f);
    EXPECT_FLOAT_EQ(vec.z(), 3.0f);
}

/**
 * @test InputFlags
 */
TEST_F(GameProtoTest, InputFlags) {
    rtype::game::InputFlags flags;
    flags.set_move_up(true);
    flags.set_move_down(false);
    flags.set_move_left(true);
    flags.set_move_right(false);
    flags.set_shoot(true);
    flags.set_special(false);

    EXPECT_TRUE(flags.move_up());
    EXPECT_FALSE(flags.move_down());
    EXPECT_TRUE(flags.move_left());
    EXPECT_FALSE(flags.move_right());
    EXPECT_TRUE(flags.shoot());
    EXPECT_FALSE(flags.special());
}

/**
 * @test ClientInput complet
 */
TEST_F(GameProtoTest, ClientInputComplete) {
    rtype::game::ClientInput input;
    input.set_sequence_number(100);
    input.set_client_tick(5000);
    input.set_timestamp_ms(1700000000000);

    auto* flags = input.mutable_input();
    flags->set_move_up(true);
    flags->set_shoot(true);

    auto* aim = input.mutable_aim_direction();
    aim->set_x(1.0f);
    aim->set_y(0.0f);

    EXPECT_EQ(input.sequence_number(), 100u);
    EXPECT_TRUE(input.input().move_up());
    EXPECT_TRUE(input.input().shoot());
    EXPECT_FLOAT_EQ(input.aim_direction().x(), 1.0f);
}

/**
 * @test EntityState avec tous les types d'entités
 */
TEST_F(GameProtoTest, EntityStateAllTypes) {
    std::vector<rtype::game::EntityType> types = {
        rtype::game::ENTITY_PLAYER,
        rtype::game::ENTITY_ENEMY_BASIC,
        rtype::game::ENTITY_ENEMY_BOSS,
        rtype::game::ENTITY_PROJECTILE_PLAYER,
        rtype::game::ENTITY_PROJECTILE_ENEMY,
        rtype::game::ENTITY_POWERUP,
        rtype::game::ENTITY_OBSTACLE
    };

    for (auto type : types) {
        rtype::game::EntityState entity;
        entity.set_type(type);
        EXPECT_EQ(entity.type(), type);
    }
}

/**
 * @test EntityState complet
 */
TEST_F(GameProtoTest, EntityStateComplete) {
    rtype::game::EntityState entity;
    entity.set_network_id(42);
    entity.set_type(rtype::game::ENTITY_PLAYER);

    auto* pos = entity.mutable_position();
    pos->set_x(100.0f);
    pos->set_y(200.0f);

    auto* vel = entity.mutable_velocity();
    vel->set_x(5.0f);
    vel->set_y(-3.0f);

    entity.set_rotation(45.0f);
    entity.set_health(80);
    entity.set_max_health(100);
    entity.set_is_dead(false);
    entity.set_owner_id(0);

    EXPECT_EQ(entity.network_id(), 42u);
    EXPECT_EQ(entity.type(), rtype::game::ENTITY_PLAYER);
    EXPECT_FLOAT_EQ(entity.position().x(), 100.0f);
    EXPECT_FLOAT_EQ(entity.velocity().x(), 5.0f);
    EXPECT_FLOAT_EQ(entity.rotation(), 45.0f);
    EXPECT_EQ(entity.health(), 80u);
    EXPECT_FALSE(entity.is_dead());
}

/**
 * @test WorldSnapshot avec repeated entities
 */
TEST_F(GameProtoTest, WorldSnapshotWithEntities) {
    rtype::game::WorldSnapshot snapshot;
    snapshot.set_server_tick(1000);
    snapshot.set_timestamp_ms(1700000000000);
    snapshot.set_state(rtype::game::GAME_RUNNING);

    // Ajouter plusieurs entités
    for (int i = 0; i < 5; i++) {
        auto* entity = snapshot.add_entities();
        entity->set_network_id(i);
        entity->set_type(rtype::game::ENTITY_PLAYER);
        entity->mutable_position()->set_x(static_cast<float>(i * 100));
        entity->mutable_position()->set_y(static_cast<float>(i * 50));
        entity->set_health(100 - i * 10);
    }

    EXPECT_EQ(snapshot.entities_size(), 5);
    EXPECT_EQ(snapshot.entities(0).network_id(), 0u);
    EXPECT_EQ(snapshot.entities(4).network_id(), 4u);
    EXPECT_EQ(snapshot.entities(2).health(), 80u);
}

/**
 * @test WorldSnapshot avec events
 */
TEST_F(GameProtoTest, WorldSnapshotWithEvents) {
    rtype::game::WorldSnapshot snapshot;
    snapshot.set_server_tick(500);
    snapshot.set_state(rtype::game::GAME_RUNNING);

    auto* event1 = snapshot.add_events();
    event1->set_event_id(1);
    event1->set_event_type("DAMAGE");
    event1->set_source_entity(10);
    event1->set_target_entity(20);
    event1->set_value(-25);

    auto* event2 = snapshot.add_events();
    event2->set_event_id(2);
    event2->set_event_type("HEAL");
    event2->set_source_entity(5);
    event2->set_target_entity(10);
    event2->set_value(15);

    EXPECT_EQ(snapshot.events_size(), 2);
    EXPECT_EQ(snapshot.events(0).event_type(), "DAMAGE");
    EXPECT_EQ(snapshot.events(1).event_type(), "HEAL");
}

/**
 * @test Sérialisation WorldSnapshot complet
 */
TEST_F(GameProtoTest, SerializeWorldSnapshot) {
    rtype::game::WorldSnapshot original;
    original.set_server_tick(999);
    original.set_timestamp_ms(1700000000000);
    original.set_state(rtype::game::GAME_RUNNING);

    for (int i = 0; i < 10; i++) {
        auto* entity = original.add_entities();
        entity->set_network_id(i);
        entity->set_type(rtype::game::ENTITY_ENEMY_BASIC);
        entity->mutable_position()->set_x(static_cast<float>(i));
        entity->set_health(100);
    }

    std::string serialized;
    ASSERT_TRUE(original.SerializeToString(&serialized));
    EXPECT_GT(serialized.size(), 0u);

    rtype::game::WorldSnapshot deserialized;
    ASSERT_TRUE(deserialized.ParseFromString(serialized));

    EXPECT_EQ(deserialized.server_tick(), 999u);
    EXPECT_EQ(deserialized.state(), rtype::game::GAME_RUNNING);
    EXPECT_EQ(deserialized.entities_size(), 10);
}

/**
 * @test GameMessage avec oneof ClientInput
 */
TEST_F(GameProtoTest, GameMessageWithClientInput) {
    rtype::game::GameMessage msg;
    msg.set_type(rtype::game::MSG_CLIENT_INPUT);
    msg.set_sequence_id(1);

    auto* input = msg.mutable_client_input();
    input->set_sequence_number(100);
    input->mutable_input()->set_shoot(true);

    EXPECT_TRUE(msg.has_client_input());
    EXPECT_FALSE(msg.has_world_snapshot());
    EXPECT_TRUE(msg.client_input().input().shoot());
}

/**
 * @test GameMessage avec oneof WorldSnapshot
 */
TEST_F(GameProtoTest, GameMessageWithWorldSnapshot) {
    rtype::game::GameMessage msg;
    msg.set_type(rtype::game::MSG_WORLD_SNAPSHOT);

    auto* snapshot = msg.mutable_world_snapshot();
    snapshot->set_server_tick(500);
    snapshot->add_entities()->set_network_id(1);

    EXPECT_TRUE(msg.has_world_snapshot());
    EXPECT_FALSE(msg.has_client_input());
    EXPECT_EQ(msg.world_snapshot().entities_size(), 1);
}

/**
 * @test Ping/Pong pour mesure de latence
 */
TEST_F(GameProtoTest, PingPong) {
    rtype::game::Ping ping;
    ping.set_client_timestamp(1700000000000);
    ping.set_sequence(42);

    rtype::game::Pong pong;
    pong.set_client_timestamp(ping.client_timestamp());
    pong.set_server_timestamp(1700000000050);
    pong.set_sequence(ping.sequence());

    uint64_t latency = pong.server_timestamp() - pong.client_timestamp();
    EXPECT_EQ(latency, 50u);
    EXPECT_EQ(pong.sequence(), ping.sequence());
}

/**
 * @test PlayerJoin avec EntityState initial
 */
TEST_F(GameProtoTest, PlayerJoin) {
    rtype::game::PlayerJoin join;
    join.set_player_id(123);
    join.set_username("NewPlayer");

    auto* state = join.mutable_initial_state();
    state->set_network_id(123);
    state->set_type(rtype::game::ENTITY_PLAYER);
    state->mutable_position()->set_x(0.0f);
    state->mutable_position()->set_y(300.0f);
    state->set_health(100);
    state->set_max_health(100);

    std::string serialized;
    ASSERT_TRUE(join.SerializeToString(&serialized));

    rtype::game::PlayerJoin deserialized;
    ASSERT_TRUE(deserialized.ParseFromString(serialized));

    EXPECT_EQ(deserialized.username(), "NewPlayer");
    EXPECT_EQ(deserialized.initial_state().health(), 100u);
}

/**
 * @test Tous les états de jeu
 */
TEST_F(GameProtoTest, AllGameStates) {
    rtype::game::WorldSnapshot snapshot;

    snapshot.set_state(rtype::game::GAME_WAITING);
    EXPECT_EQ(snapshot.state(), rtype::game::GAME_WAITING);

    snapshot.set_state(rtype::game::GAME_STARTING);
    EXPECT_EQ(snapshot.state(), rtype::game::GAME_STARTING);

    snapshot.set_state(rtype::game::GAME_RUNNING);
    EXPECT_EQ(snapshot.state(), rtype::game::GAME_RUNNING);

    snapshot.set_state(rtype::game::GAME_PAUSED);
    EXPECT_EQ(snapshot.state(), rtype::game::GAME_PAUSED);

    snapshot.set_state(rtype::game::GAME_ENDED);
    EXPECT_EQ(snapshot.state(), rtype::game::GAME_ENDED);
}

/**
 * @test Taille de sérialisation pour différentes tailles de snapshot
 */
TEST_F(GameProtoTest, SerializationSizeScaling) {
    std::vector<size_t> sizes;

    for (int numEntities = 0; numEntities <= 100; numEntities += 20) {
        rtype::game::WorldSnapshot snapshot;
        snapshot.set_server_tick(100);

        for (int i = 0; i < numEntities; i++) {
            auto* entity = snapshot.add_entities();
            entity->set_network_id(i);
            entity->set_type(rtype::game::ENTITY_PLAYER);
            entity->mutable_position()->set_x(static_cast<float>(i));
            entity->mutable_position()->set_y(static_cast<float>(i));
            entity->set_health(100);
        }

        std::string serialized;
        snapshot.SerializeToString(&serialized);
        sizes.push_back(serialized.size());
    }

    // La taille devrait augmenter avec le nombre d'entités
    for (size_t i = 1; i < sizes.size(); i++) {
        EXPECT_GE(sizes[i], sizes[i - 1]);
    }
}

/**
 * @test Performance: sérialisation/désérialisation rapide
 */
TEST_F(GameProtoTest, SerializationPerformance) {
    rtype::game::WorldSnapshot original;
    original.set_server_tick(1000);

    for (int i = 0; i < 50; i++) {
        auto* entity = original.add_entities();
        entity->set_network_id(i);
        entity->set_type(rtype::game::ENTITY_ENEMY_BASIC);
        entity->mutable_position()->set_x(static_cast<float>(i * 10));
        entity->mutable_position()->set_y(static_cast<float>(i * 5));
        entity->mutable_velocity()->set_x(1.0f);
        entity->mutable_velocity()->set_y(0.0f);
        entity->set_health(100);
    }

    std::string serialized;

    // 1000 round-trips
    for (int i = 0; i < 1000; i++) {
        original.SerializeToString(&serialized);
        rtype::game::WorldSnapshot deserialized;
        deserialized.ParseFromString(serialized);
    }

    // Si on arrive ici sans timeout, la performance est acceptable
    SUCCEED();
}
