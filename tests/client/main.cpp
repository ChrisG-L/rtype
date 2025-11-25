/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Point d'entrée des tests unitaires du client
*/

/**
 * @file main.cpp
 * @brief Point d'entrée pour les tests unitaires du client R-Type
 *
 * Ce fichier initialise Google Test et exécute tous les tests
 * du client incluant :
 * - Tests des Utilitaires (Vecs, Signal)
 * - Tests des composants UI (Button, TextField)
 * - Tests des systèmes (SceneManager, etc.)
 *
 * @usage
 *   ./client_tests              # Exécute tous les tests
 *   ./client_tests --gtest_filter=VecsTest.*     # Tests Vecs seulement
 *   ./client_tests --gtest_list_tests            # Liste tous les tests
 */

#include <gtest/gtest.h>

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
