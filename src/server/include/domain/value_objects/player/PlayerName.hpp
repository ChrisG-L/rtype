/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerName
*/

#ifndef PLAYERNAME_HPP_
#define PLAYERNAME_HPP_

#include "domain/exceptions/player/PlayerNameException.hpp"
// #include <bsoncxx/exception/exception.hpp>

namespace domain::value_objects::player {
    class PlayerName {
        private:
            std::string _name;
            void validate(const std::string &name);

        public:
            explicit PlayerName(const std::string& name);
            std::string value() const;
            bool operator==(const PlayerName& other);
            bool operator!=(const PlayerName& other);
    };
}

#endif /* !PLAYERNAME_HPP_ */
