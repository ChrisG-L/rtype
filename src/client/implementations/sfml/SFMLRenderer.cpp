/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** SFMLRenderer
*/

#include "implementations/sfml/SFMLRenderer.hpp"

SFMLRenderer::SFMLRenderer(graphics::IWindow* window): _window{window}, mAsset{std::make_unique<AssetManager>()}
{
}

void SFMLRenderer::initialize()
{
    // Image test
    std::string bedRoomFile = "assets/spaceship/bedroom.jpg";

    mAsset->registerTexture(bedRoomFile);

    sf::Sprite bedRoomSprite(mAsset->getTexture(bedRoomFile));
    mAsset->addSprite(bedRoomFile, bedRoomSprite);
}

void SFMLRenderer::update()
{
    mAsset->drawAll(_window);
}
