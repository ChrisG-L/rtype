/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** AssetManager
*/

#include "utils/AssetManager.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <stdexcept>
#include <algorithm>
#include <vector>

AssetManager::AssetManager(): textures{}
{
}

sf::Texture& AssetManager::getTexture(const std::string& key)
{
    auto textIt = textures.find(key);
    if (textIt == textures.end())
        throw std::out_of_range("Texture non chargée.");
    return textIt->second;
}

sf::Sprite& AssetManager::getSprite(const std::string& textureKey, const std::string& spriteKey)
{
    auto textIt = textures.find(textureKey);
    if (textIt == textures.end())
        throw std::out_of_range("Texture non chargée.");
    auto poolIt = spritePools.find(textureKey);
    if (poolIt == spritePools.end())
        throw std::out_of_range("Sprite pool non trouvé.");
    auto spriteIt = poolIt->second.find(spriteKey);
    if (spriteIt == poolIt->second.end())
        throw std::out_of_range("Sprite non trouvé.");
    return spriteIt->second.sprite;
}


bool AssetManager::registerTexture(graphic::GraphicTexture& graphT, const std::string& file)
{
    auto logger = client::logging::Logger::getGraphicsLogger();

    if (textures.count(file)) {
        logger->debug("Texture '{}' already registered", file);
        return false;
    }

    sf::Texture newTexture;

    if (!newTexture.loadFromFile(file)) {
        logger->error("Failed to load texture from file: {}", file);
        return false;
    }
    sf::Vector2u textureSize = newTexture.getSize();
    graphT.setSize({textureSize.x, textureSize.y});
    textures.emplace(file, std::move(newTexture));
    logger->debug("Texture '{}' registered successfully", file);
    return true;
}

void AssetManager::removeTexture(const std::string& file)
{
    auto logger = client::logging::Logger::getGraphicsLogger();

    if (textures.count(file)) {
        textures.erase(file);
        logger->debug("Texture '{}' removed", file);
    } else {
        logger->warn("Texture '{}' not found for removal", file);
    }
}

void AssetManager::addSprite(const std::string& textureKey, const std::string& spriteKey, const sf::Sprite& sprite, int zIndex)
{
    SpriteData spriteData = {.sprite = sprite, .zIndex  = zIndex};
    spritePools[textureKey].insert_or_assign(spriteKey, spriteData);
}

void AssetManager::setSprite(const std::string& textureKey, const std::string& spriteKey, const sf::Sprite& sprite, int zIndex)
{
    SpriteData spriteData = {.sprite = sprite, .zIndex  = zIndex};
    spritePools[textureKey].insert_or_assign(spriteKey, spriteData);
}

void AssetManager::drawAll(sf::RenderWindow* window)
{
    std::vector<const SpriteData*> sortedSprites;

    for (const auto& [texture, spritesPool] : spritePools) {
        for (const auto& [key, spriteData] : spritesPool) {
            sortedSprites.push_back(&spriteData);
        }
    }

    std::sort(sortedSprites.begin(), sortedSprites.end(),
        [](const SpriteData* a, const SpriteData* b) {
            return a->zIndex < b->zIndex;
        });

    for (const auto* spriteData : sortedSprites) {
        window->draw(spriteData->sprite);
    }
}
