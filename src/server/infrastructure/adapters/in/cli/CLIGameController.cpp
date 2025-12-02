/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** CLIGameController
*/

#include "infrastructure/adapters/in/cli/CLIGameController.hpp"

namespace infrastructure::adapters::in::cli {
    void CLIGameController::movePlayer(const std::string& playerId, float dx, float dy, float dz) {
        move.execute(PlayerId(playerId), dx, dy, dz);
    }
}
