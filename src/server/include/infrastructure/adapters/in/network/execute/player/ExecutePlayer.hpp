/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ExecutePlayer
*/

#ifndef EXECUTEPLAYER_HPP_
#define EXECUTEPLAYER_HPP_

#include <iostream>
#include <memory>

#include "../../protocol/Command.hpp"
#include "application/use_cases/player/Move.hpp"
#include "infrastructure/adapters/out/persistence/MongoDBUserRepository.hpp"
#include "Protocol.hpp"


namespace infrastructure::adapters::in::network::execute::player {
    using application::use_cases::player::Move;
    using domain::value_objects::player::PlayerId;

    class ExecutePlayer {
        public:
            ExecutePlayer(
                const Command& cmd,
                std::shared_ptr<Move> movePlayer
            );

        private:
            Command _cmd;
            std::shared_ptr<Move> _movePlayer;

            void move();
    };
}


#endif /* !EXECUTEPLAYER_HPP_ */
