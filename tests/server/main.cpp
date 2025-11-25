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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
