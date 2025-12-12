/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SDL2Renderer
*/

#include "SDL2Renderer.hpp"

SDL2Renderer::SDL2Renderer(std::shared_ptr<graphics::IWindow> window)
    : _window(window)
{
    _sdlRenderer = static_cast<SDL_Renderer*>(_window->getNativeHandle());
    if (!_sdlRenderer) {
        throw std::runtime_error("Failed to get SDL renderer from window");
    }
    _assetManager = std::make_unique<SDL2AssetManager>(_sdlRenderer);
}

void SDL2Renderer::initialize(GraphicAssets& assets, GraphicAssetsE& elements)
{
    _elements = &elements;

    for (const auto& asset : assets) {
        std::visit(overloaded {
            [&](const graphic::GraphicTexture& graphT) { initGraphicTexture(graphT); }
        }, asset);
    }

    for (const auto& elem : elements) {
        std::visit(overloaded {
            [&](const graphic::GraphicElement& graphE) { initGraphicElement(graphE); }
        }, elem);
    }
}

void SDL2Renderer::update(float deltatime, GraphicAssets& assets, GraphicAssetsE& elements)
{
    _elements = &elements;
}

void SDL2Renderer::render()
{
    _assetManager->drawAll();
}

void SDL2Renderer::initGraphicTexture(const graphic::GraphicTexture& textureAsset)
{
    std::string path = textureAsset.getFileName();
    if (!_assetManager->registerTexture(path)) {
        throw std::runtime_error("Texture provided not found: " + path);
    }
}

void SDL2Renderer::initGraphicElement(const graphic::GraphicElement& elem)
{
    std::string path = elem.getTexture().getFileName();
    Vec2f pos = elem.getPos();
    Vec2f scale = elem.getScale();

    _assetManager->addSprite(path, pos.x, pos.y, scale.x, scale.y);
}
