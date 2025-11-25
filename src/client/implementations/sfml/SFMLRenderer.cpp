/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** SFMLRenderer
*/

#include "implementations/sfml/SFMLRenderer.hpp"
#include "graphics/Graphics.hpp"
#include "utils/Vecs.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
#include <stdexcept>
#include <variant>

void test() {}

SFMLRenderer::SFMLRenderer(std::shared_ptr<graphics::IWindow> window)
    : _window{window}, mAsset{std::make_unique<AssetManager>()}
{
}

void SFMLRenderer::initialize(GraphicAssets assets)
{
    for (const auto& a: assets) {
        std::visit(overloaded {
            [&](const graphic::GraphicTexture& graphT) {initGraphicTexture(graphT);}
        }, a);
    }
}

void SFMLRenderer::update()
{
    // mAsset->drawAll(_window);
}

void SFMLRenderer::render()
{
    mAsset->drawAll(_window);
}

void SFMLRenderer::initGraphicTexture(const graphic::GraphicTexture& textureAsset) {
    std::string path = textureAsset.getFileName();
    if (!mAsset->registerTexture(path))
        throw std::runtime_error("Texture provided not found");
    Vec2f pos = textureAsset.getPos();
    Vec2f scale = textureAsset.getScale();
    
    sf::Sprite sprite(mAsset->getTexture(path));
    sprite.setPosition(sf::Vector2f(pos.x, pos.y));
    sprite.scale(sf::Vector2f(scale.x, scale.y));
    mAsset->addSprite(path, sprite);
}