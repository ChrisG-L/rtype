/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Player
*/

#ifndef PLAYER_HPP_
#define PLAYER_HPP_

#include "domain/value_objects/Health.hpp"
#include "domain/value_objects/player/PlayerId.hpp"
#include "domain/value_objects/Position.hpp"

namespace domain::entities {
    class Player {
        private:
            value_objects::Health _health;
            value_objects::player::PlayerId _id;
            value_objects::Position _position;

        public:
            explicit Player(
                value_objects::Health health,
                value_objects::player::PlayerId id,
                value_objects::Position position = value_objects::Position()
            );

            const value_objects::player::PlayerId& getId() const;
            const value_objects::Position& getPosition() const;
            void move(float dx, float dy, float dz);
            void heal(float value);
            void takeDamage(float value);
    };
}

#endif /* !PLAYER_HPP_ */
