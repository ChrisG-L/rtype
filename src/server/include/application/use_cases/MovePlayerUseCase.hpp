/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MovePlayerUseCase
*/

#ifndef MOVEPLAYERUSECASE_HPP_
#define MOVEPLAYERUSECASE_HPP_

#include "domain/entities/Player.hpp"
#include "application/ports/out/persistence/IPlayerRepository.hpp"


namespace application::use_cases {
    using domain::value_objects::player::PlayerId;
    using application::ports::out::persistence::IPlayerRepository;

    class MovePlayerUseCase {
        private:
            IPlayerRepository* repository;
        public:
            explicit MovePlayerUseCase(IPlayerRepository* repo) : repository(repo) {}

            void execute(const PlayerId& id, float dx, float dy, float dz) {
                auto playerOpt = repository->findById(id.value());
                if (!playerOpt.has_value()) {
                    return; // Player not found, do nothing
                }

                auto player = playerOpt.value();
                player.move(dx, dy, dz);
                repository->update(player);
            };

        protected:
    };
}

#endif /* !MOVEPLAYERUSECASE_HPP_ */
