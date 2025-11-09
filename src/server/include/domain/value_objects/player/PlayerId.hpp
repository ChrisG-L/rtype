/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerId
*/

#ifndef PLAYERID_HPP_
#define PLAYERID_HPP_

#include "PlayerIdException.hpp"
#include <bsoncxx/oid.hpp>
#include <bsoncxx/exception/exception.hpp>

namespace domain::value_objects::player {
    class PlayerId {
        private:
            std::string _id;

            void validate(const std::string &id) {
                try {
                    bsoncxx::oid _{id};
                }
                catch(const bsoncxx::exception& ex) {
                    throw exceptions::player::PlayerIdException(id);
                }
            }
        public:
            explicit PlayerId(const std::string& id): _id(id) {
                validate(id);
            };

            std::string value() const {
                return _id;
            }

            bool operator==(const PlayerId& other) {
                return _id == other._id;
            }

            bool operator!=(const PlayerId& other) {
                return !(*this != other);
            }

        protected:
    };
}

#endif /* !PLAYERID_HPP_ */
