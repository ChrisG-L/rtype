/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Graphics
*/

#include "graphics/Graphics.hpp"

namespace graphic {
    GraphicTexture::GraphicTexture(const std::string& filename, Vec2f pos, Vec2f scale)
    : _filename(filename), _pos(pos), _scale(scale)
    {
    }

    std::string GraphicTexture::getFileName() const {
        return _filename;
    }

    Vec2f GraphicTexture::getPos() const {
        return _pos;
    }

    Vec2f GraphicTexture::getScale() const {
        return _scale;
    }

}

