/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Player
*/

#include "domain/entities/Player.hpp"

namespace domain::entities {
    Player::Player(
        value_objects::Health health,
        value_objects::player::PlayerId id,
        value_objects::Position position
    ) : _health(health), _id(id), _position(position) {}

    const value_objects::player::PlayerId& Player::getId() const {
        return _id;
    }

    const value_objects::Position& Player::getPosition() const {
        return _position;
    }

    void Player::move(float dx, float dy, float dz) {
        _position = _position.move(dx, dy, dz);
    }

    void Player::heal(float value) {
        _health = _health.heal(value);
    }

    void Player::takeDamage(float value) {
        _health = _health.damage(value);
    }
}
