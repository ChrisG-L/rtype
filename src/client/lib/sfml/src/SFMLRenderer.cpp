/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** SFMLRenderer
*/

#include "SFMLRenderer.hpp"
#include "graphics/Graphics.hpp"
#include "utils/Vecs.hpp"

SFMLRenderer::SFMLRenderer(std::shared_ptr<graphics::IWindow> window)
    : _window{window}, mAsset{std::make_unique<AssetManager>()}
{
}

void SFMLRenderer::initialize(GraphicAssets& assets, GraphicAssetsE& elements)
{
    for (auto& a: assets) {
        std::visit(overloaded {
            [&](graphic::GraphicTexture& graphT) {initGraphicTexture(graphT);}
        }, a);
    }
    for (auto& e: elements) {
        std::visit(overloaded {
            [&](graphic::GraphicElement& element) {initGraphicElement(element);}
        }, e);
    }
}

void SFMLRenderer::update(float deltatime, GraphicAssets& assets, GraphicAssetsE& elements)
{
    for (auto& a: assets) {
        std::visit(overloaded {
            [&](graphic::GraphicTexture& graphT) {setGraphicTexture(graphT);}
        }, a);
    }
    for (auto& e: elements) {
        std::visit(overloaded {
            [&](graphic::GraphicElement& element) {setGraphicElement(element);}
        }, e);
    }
}

void SFMLRenderer::render()
{
    auto* renderWindow = static_cast<sf::RenderWindow*>(_window->getNativeHandle());
    mAsset->drawAll(renderWindow);
}

void SFMLRenderer::initGraphicTexture(graphic::GraphicTexture& textureAsset) {
    std::string path = textureAsset.getFileName();
    if (!mAsset->registerTexture(textureAsset, path))
        throw std::runtime_error("Texture provided not found");
}


void SFMLRenderer::initGraphicElement(graphic::GraphicElement& element) {
    Vec2f pos = element.getPos();
    Vec2f scale = element.getScale();
    std::string path = element.getTexture().getFileName();
    sf::Sprite sprite(mAsset->getTexture(path));
    sprite.setPosition(sf::Vector2f(pos.x, pos.y));
    sprite.scale(sf::Vector2f(scale.x, scale.y));
    mAsset->addSprite(path, element.getName(), sprite);
}

void SFMLRenderer::setGraphicTexture(graphic::GraphicTexture& textureAsset) {
    // std::string path = textureAsset.getFileName();
    // // if (!mAsset->registerTexture(path))
    // //     throw std::runtime_error("Texture provided not found");
    // Vec2f pos = textureAsset.getPos();
    // Vec2f scale = textureAsset.getScale();
    // sf::Sprite sprite(mAsset->getTexture(path));
    // sprite.setPosition(sf::Vector2f(pos.x, pos.y));
    // sprite.scale(sf::Vector2f(scale.x, scale.y));
    // mAsset->addSprite(path, sprite);
}

void SFMLRenderer::setGraphicElement(graphic::GraphicElement& element) {
    std::string path = element.getTexture().getFileName();
    Vec2f pos = element.getPos();
    Vec2f scale = element.getScale();
    sf::Sprite sprite(mAsset->getTexture(path));
    sprite.setPosition(sf::Vector2f(pos.x, pos.y));
    sprite.scale(sf::Vector2f(scale.x, scale.y));
    mAsset->setSprite(path, element.getName(), sprite);
}