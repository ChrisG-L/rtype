/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MovePlayerUseCase
*/

#include "application/use_cases/MovePlayerUseCase.hpp"

namespace application::use_cases {
    MovePlayerUseCase::MovePlayerUseCase(IPlayerRepository* repo)
        : repository(repo) {}

    void MovePlayerUseCase::execute(const PlayerId& id, float dx, float dy, float dz) {
        auto playerOpt = repository->findById(id.value());
        if (!playerOpt.has_value()) {
            return;
        }

        auto player = playerOpt.value();
        player.move(dx, dy, dz);
        repository->update(player);
    }
}
