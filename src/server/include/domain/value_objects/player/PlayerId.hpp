/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerId
*/

#ifndef PLAYERID_HPP_
#define PLAYERID_HPP_

#include "domain/exceptions/player/PlayerIdException.hpp"
#include <bsoncxx/oid.hpp>
#include <bsoncxx/exception/exception.hpp>

namespace domain::value_objects::player {
    class PlayerId {
        private:
            std::string _id;
            void validate(const std::string &id);

        public:
            explicit PlayerId(const std::string& id);
            std::string value() const;
            bool operator==(const PlayerId& other);
            bool operator!=(const PlayerId& other);
    };
}

#endif /* !PLAYERID_HPP_ */
