/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Graphics
*/

#include "graphics/Graphics.hpp"
#include "graphics/Asset.hpp"
#include "utils/Vecs.hpp"

namespace graphic {

    GraphicTexture::GraphicTexture(const std::string& pathName, const std::string& name)
    : _pathName(pathName), _name(name), _size({1, 1})
    {
    }

    std::string GraphicTexture::getFileName() const {
        return _pathName;
    }

    std::string GraphicTexture::getName() const {
        return _name;
    }


    Vec2u GraphicTexture::getSize() const {
        return _size;
    }


    void GraphicTexture::setSize(Vec2u size) {
        _size = size;
    }

    // Graphic Element

    GraphicElement::GraphicElement(
        const GraphicTexture& texture,
        Vec2f pos,
        Vec2f scale,
        std::string name,
        Layer layer
    ) : _texture(texture), _pos(pos), _scale(scale), _name(name), _isAlive(true), _layer(layer)
    {
    }

    const GraphicTexture& GraphicElement::getTexture() const {
        return _texture;
    }

    std::string GraphicElement::getName() const {
        return _name;
    }

    Vec2f GraphicElement::getPos() const {
        return _pos;
    }
    
    Vec2f GraphicElement::getScale() const {
        return _scale;
    }

    bool GraphicElement::isAlive() const {
        return _isAlive;
    }

    void GraphicElement::setTexture(const GraphicTexture& texture) {
        _texture = texture;
    }

    void GraphicElement::setPos(Vec2f pos) {
        _pos = pos;
    }

    void GraphicElement::setScale(Vec2f scale) {
        _scale = scale;
    }

    void GraphicElement::setDie() {
        _isAlive = false;
    }
}