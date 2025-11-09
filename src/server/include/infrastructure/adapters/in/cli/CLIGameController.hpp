/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** CLIGameController
*/

#ifndef CLIGAMECONTROLLER_HPP_
#define CLIGAMECONTROLLER_HPP_

#include "domain/value_objects/player/PlayerId.hpp"
#include "application/ports/in/IGameCommands.hpp"
#include "application/use_cases/MovePlayerUseCase.hpp"


namespace infrastructure::adapters::in::cli {
    using application::ports::in::IGameCommands;
    using domain::value_objects::player::PlayerId;
    class CLIGameController: public IGameCommands {
        public:
            void movePlayer(const std::string& playerId, float dx, float dy, float dz = 0.0) {
                movePlayerUseCase.execute(PlayerId(playerId), dx, dy, dz);
            }
        protected:
        private:
            application::use_cases::MovePlayerUseCase movePlayerUseCase;
    };
}

#endif /* !CLIGAMECONTROLLER_HPP_ */
