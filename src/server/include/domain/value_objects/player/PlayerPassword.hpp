/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerPassword
*/

#ifndef PLAYERPASSWORD_HPP_
#define PLAYERPASSWORD_HPP_

#include "domain/exceptions/player/PlayerPasswordException.hpp"
#include <bsoncxx/exception/exception.hpp>

namespace domain::value_objects::player {
    class PlayerPassword {
        private:
            std::string _password;
            void validate(const std::string &password);

        public:
            explicit PlayerPassword(const std::string& password);
            std::string value() const;
            bool operator==(const PlayerPassword& other);
            bool operator!=(const PlayerPassword& other);
    };
}

#endif /* !PLAYERPASSWORD_HPP_ */
