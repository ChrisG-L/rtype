/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Point d'entrée des tests unitaires du serveur
*/

/**
 * @file main.cpp
 * @brief Point d'entrée pour les tests unitaires du serveur R-Type
 *
 * Ce fichier initialise Google Test et exécute tous les tests
 * du serveur incluant :
 * - Tests des Value Objects (Health, Position, Email, Username, etc.)
 * - Tests des Entités (Player, User)
 * - Tests des Use Cases (MovePlayer, Login, Register)
 *
 * @usage
 *   ./server_tests              # Exécute tous les tests
 *   ./server_tests --gtest_filter=HealthTest.*   # Tests Health seulement
 *   ./server_tests --gtest_list_tests            # Liste tous les tests
 */

#include <gtest/gtest.h>
#include "infrastructure/logging/Logger.hpp"

int main(int argc, char **argv) {
    // Initialize logger for tests that depend on it (e.g., FriendManager)
    server::logging::Logger::init();
    server::logging::Logger::setLevel(spdlog::level::off);  // Silence logs during tests

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    server::logging::Logger::shutdown();
    return result;
}
