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
#include "application/use_cases/player/Move.hpp"


namespace infrastructure::adapters::in::cli {
    using application::ports::in::IGameCommands;
    using domain::value_objects::player::PlayerId;

    class CLIGameController: public IGameCommands {
        private:
            application::use_cases::player::Move move;

        public:
            void movePlayer(const std::string& playerId, float dx, float dy, float dz = 0.0) override;
    };
}

#endif /* !CLIGAMECONTROLLER_HPP_ */
