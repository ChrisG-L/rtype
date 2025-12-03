/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Missile
*/

#include "gameplay/Missile.hpp"

namespace gameplay {

    Missile::Missile(const graphic::GraphicTexture& texture, std::string tag, Vec2f spawnPos, float speed)
        : GameObject(texture, spawnPos, {1.0f, 1.0f}, tag, graphic::Layer::Projectiles)
        , _speed(speed)
    {
        _velocity = {_speed, 0};
    }

    void Missile::update(float deltaTime)
    {
        _position.x += _velocity.x * deltaTime;
        _position.y += _velocity.y * deltaTime;
        _element.setPos(_position);
        if (_position.x > SCREEN_WIDTH) {
            destroy();
            _element.setDie();
        }
    }
}
