/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Player
*/

#ifndef PLAYER_HPP_
#define PLAYER_HPP_

#include "value_objects/Health.hpp"
#include "value_objects/player/PlayerId.hpp"
#include "value_objects/Position.hpp"

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
            ): _health(health), _id(id), _position(position) {

            };

            // Getter
            const value_objects::player::PlayerId& getId() const {
                return _id;
            }

            const value_objects::Position& getPosition() const {
                return _position;
            }

            // Setter
            void move(float dx, float dy, float dz) {
                _position = _position.move(dx, dy, dz);
            }
            void heal(float value) {
                _health = _health.heal(value);
            }
            
            void takeDamage(float value) {
                _health = _health.damage(value);
            }

        protected:
    };
}

#endif /* !PLAYER_HPP_ */
