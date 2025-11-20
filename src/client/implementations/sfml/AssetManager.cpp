/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** AssetManager
*/

#include "implementations/sfml/utils/AssetManager.hpp"
#include "core/Logger.hpp"
#include <memory>

AssetManager::AssetManager()
{
}

sf::Texture& AssetManager::getTexture(const std::string& key)
{
    auto textIt = textures.find(key);
    if (textIt == textures.end())
        throw std::out_of_range("Texture non chargée.");
    return textIt->second;
}

bool AssetManager::registerTexture(const std::string& file)
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

void AssetManager::addSprite(const std::string& key, const sf::Sprite& sprite)
{
    spritePools[key].push_back(sprite);
}

void AssetManager::drawAll(std::shared_ptr<graphics::IWindow> window)
{
    for (auto const& [key, sprites]: spritePools) {
        for (const auto& sprite: sprites) {
            window->draw(sprite);
        }
    }
}
