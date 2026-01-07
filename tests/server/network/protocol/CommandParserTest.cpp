/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour le CommandParser
*/

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "infrastructure/adapters/in/network/protocol/CommandParser.hpp"
#include "infrastructure/adapters/in/network/protocol/Command.hpp"

using namespace infrastructure::adapters::in::network::protocol;

/**
 * @brief Suite de tests pour le CommandParser
 *
 * Le CommandParser parse des données brutes en structure Command.
 * Command contient:
 * - uint16_t type: le type de message
 * - std::vector<uint8_t> buf: les données brutes
 *
 * Note: L'implémentation actuelle est un stub qui retourne une Command vide.
 */

class CommandParserTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Tests de la Structure Command
// ============================================================================

/**
 * @test Command par défaut a un type à 0
 */
TEST_F(CommandParserTest, DefaultCommandTypeIsZero) {
    Command cmd;
    EXPECT_EQ(cmd.type, 0u);
}

/**
 * @test Command par défaut a un buffer vide
 */
TEST_F(CommandParserTest, DefaultCommandBufferIsEmpty) {
    Command cmd;
    EXPECT_TRUE(cmd.buf.empty());
}

/**
 * @test Parsing d'une chaîne vide retourne une Command vide
 */
TEST_F(CommandParserTest, ParseEmptyStringReturnsEmptyCommand) {
    Command cmd = CommandParser::parse("");

    EXPECT_EQ(cmd.type, 0u);
    EXPECT_TRUE(cmd.buf.empty());
}

/**
 * @test Parsing d'une chaîne quelconque (stub implementation)
 */
TEST_F(CommandParserTest, ParseAnyStringReturnsEmptyCommand) {
    Command cmd = CommandParser::parse("some data");

    // L'implémentation actuelle est un stub
    EXPECT_EQ(cmd.type, 0u);
    EXPECT_TRUE(cmd.buf.empty());
}

// ============================================================================
// Tests de la Structure Command - Modification
// ============================================================================

/**
 * @test On peut modifier le type de Command
 */
TEST_F(CommandParserTest, CommandTypeCanBeModified) {
    Command cmd;
    cmd.type = 0x0060;  // MovePlayer

    EXPECT_EQ(cmd.type, 0x0060u);
}

/**
 * @test On peut modifier le buffer de Command
 */
TEST_F(CommandParserTest, CommandBufferCanBeModified) {
    Command cmd;
    cmd.buf = {0x01, 0x02, 0x03, 0x04};

    EXPECT_EQ(cmd.buf.size(), 4u);
    EXPECT_EQ(cmd.buf[0], 0x01);
    EXPECT_EQ(cmd.buf[3], 0x04);
}

/**
 * @test Command peut stocker des données binaires
 */
TEST_F(CommandParserTest, CommandCanStoreBinaryData) {
    Command cmd;
    cmd.type = 0x0040;  // Snapshot

    // Simuler un petit paquet binaire
    std::vector<uint8_t> data = {
        0x00, 0x40,  // type
        0x00, 0x01,  // sequence
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // timestamp
    };
    cmd.buf = data;

    EXPECT_EQ(cmd.type, 0x0040u);
    EXPECT_EQ(cmd.buf.size(), 12u);
}
