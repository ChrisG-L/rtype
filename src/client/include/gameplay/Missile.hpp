/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Missile - Projectile entity
*/

#ifndef MISSILE_HPP_
#define MISSILE_HPP_

#include "gameplay/GameObject.hpp"

namespace gameplay {

    class Missile : public GameObject {
        public:
            Missile(const graphic::GraphicTexture& texture, std::string tag, Vec2f spawnPos, float speed = 800.0f);
            ~Missile() override = default;

            void update(float deltaTime) override;

            float getSpeed() const { return _speed; }
            void setSpeed(float speed) { _speed = speed; }

        private:
            float _speed;
            static constexpr float SCREEN_WIDTH = 1920.0f;
    };

}

#endif /* !MISSILE_HPP_ */
