/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Move
*/

#include "application/use_cases/player/Move.hpp"

namespace application::use_cases::player {
    Move::Move() {}

    void Move::execute(const PlayerId& id, float dx, float dy, float dz) {
        // auto playerOpt = repository->findById(id.value());
        // if (!playerOpt.has_value()) {
        //     return;
        // }

        // auto player = playerOpt.value();
        // player.move(dx, dy, dz);
        // repository->update(player);
        std::cout << "inside execute: dx: " << dx << " dy: " << " dz: " << dz << std::endl;
    }
}
