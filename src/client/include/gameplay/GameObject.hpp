/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** GameObject - Base class for all game entities
*/

#ifndef GAMEOBJECT_HPP_
#define GAMEOBJECT_HPP_

#include "utils/Vecs.hpp"
#include "graphics/Graphics.hpp"
#include <string>
#include <memory>

namespace gameplay {

    class GameObject {
        public:
            GameObject(const graphic::GraphicTexture& texture, Vec2f pos, Vec2f scale, const std::string& tag, graphic::Layer layer = graphic::Layer::Entities);
            virtual ~GameObject() = default;

            virtual void update(float deltaTime) = 0;
            virtual bool isAlive() const { return _alive; }
            virtual void destroy() { _alive = false; }

            Vec2f getPosition() const { return _position; }
            Vec2f getVelocity() const { return _velocity; }
            Vec2f getScale() const { return _scale; }
            const std::string& getTag() const { return _tag; }

            void setPosition(Vec2f pos) { _position = pos; }
            void setVelocity(Vec2f vel) { _velocity = vel; }
            void setScale(Vec2f scale) { _scale = scale; }

            graphic::GraphicElement& getGraphicElement() { return _element; }
            const graphic::GraphicElement& getGraphicElement() const { return _element; }

        protected:
            Vec2f _position;
            Vec2f _velocity;
            Vec2f _scale;
            std::string _tag;
            bool _alive = true;
            graphic::GraphicElement _element;
    };

}

#endif /* !GAMEOBJECT_HPP_ */
