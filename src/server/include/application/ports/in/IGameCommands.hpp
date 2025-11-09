/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** IGameCommands
*/

#ifndef IGAMECOMMANDS_HPP_
#define IGAMECOMMANDS_HPP_

#include <string>

namespace application::ports::in {
    class IGameCommands {
        public:
            virtual ~IGameCommands() = default;

            virtual void movePlayer(const std::string& playerId, float dx, float dy, float dz = 0.0) = 0;
    };
}

#endif /* !IGAMECOMMANDS_HPP_ */
