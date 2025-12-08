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

void SDL2Renderer::initialize(GraphicAssets assets)
{
    for (const auto& asset : assets) {
        std::visit(overloaded {
            [&](const graphic::GraphicTexture& graphT) { initGraphicTexture(graphT); }
        }, asset);
    }
}

void SDL2Renderer::update(float deltatime)
{
    // Update logic if needed
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

    Vec2f pos = textureAsset.getPos();
    Vec2f scale = textureAsset.getScale();

    _assetManager->addSprite(path, pos.x, pos.y, scale.x, scale.y);
}
