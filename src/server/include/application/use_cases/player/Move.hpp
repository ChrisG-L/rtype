/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Move
*/

#ifndef MOVE_HPP_
#define MOVE_HPP_

#include "domain/value_objects/player/PlayerId.hpp"
#include "application/ports/out/persistence/IPlayerRepository.hpp"
#include <iostream>

namespace application::use_cases::player {
    using domain::value_objects::player::PlayerId;

    class Move {
        private:

        public:
            explicit Move();
            void execute(const PlayerId& id, float dx, float dy, float dz);
    };
}

#endif /* !MOVE_HPP_ */
