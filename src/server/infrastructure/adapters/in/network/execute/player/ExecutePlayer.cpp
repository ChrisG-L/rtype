/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** ExecuteAuth
*/

#include "infrastructure/adapters/in/network/execute/player/ExecutePlayer.hpp"
#include "Protocol.hpp"
#include "application/use_cases/auth/Login.hpp"

namespace infrastructure::adapters::in::network::execute::player {
    ExecutePlayer::ExecutePlayer(
        const Command& cmd,
        std::shared_ptr<Move> movePlayer
    ): _cmd(cmd), _movePlayer(movePlayer)
    {
        if (_cmd.type == static_cast<uint16_t>(MessageType::MovePlayer)) {
            move();
        } else {
            std::cout << "COMMAND NOT FOUND!" << std::endl;
        }
    }

    void ExecutePlayer::move() {
        MovePlayer movePlayer = MovePlayer::from_bytes(_cmd.buf.data());
        _movePlayer->execute(PlayerId("1"), movePlayer.x, movePlayer.y, 0);
    }
}



