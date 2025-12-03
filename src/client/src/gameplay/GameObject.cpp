/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** GameObject
*/

#include "gameplay/GameObject.hpp"

namespace gameplay {

    GameObject::GameObject(const graphic::GraphicTexture& texture, Vec2f pos, Vec2f scale, const std::string& tag, graphic::Layer layer)
        : _position(pos)
        , _velocity({0, 0})
        , _scale(scale)
        , _tag(tag)
        , _element(texture, pos, scale, tag, layer)
    {
    }

}
