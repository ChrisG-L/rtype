/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** ExecutePlayer
*/

#include "infrastructure/adapters/in/network/execute/player/ExecutePlayer.hpp"
#include "infrastructure/logging/Logger.hpp"
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
            server::logging::Logger::getNetworkLogger()->warn("Command not found: type={}", _cmd.type);
        }
    }

    void ExecutePlayer::move() {
        auto movePlayerOpt = MovePlayer::from_bytes(_cmd.buf.data(), _cmd.buf.size());
        if (!movePlayerOpt) {
            server::logging::Logger::getNetworkLogger()->warn("Invalid MovePlayer message received!");
            return;
        }
        _movePlayer->execute(PlayerId("1"), movePlayerOpt->x, movePlayerOpt->y, 0);
    }
}



