/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SDL2AssetManager
*/

#include "utils/SDL2AssetManager.hpp"
#include <stdexcept>

SDL2AssetManager::SDL2AssetManager(SDL_Renderer* renderer)
    : _renderer(renderer)
{
    int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        throw std::runtime_error("Failed to initialize SDL_image: " + std::string(IMG_GetError()));
    }
}

SDL2AssetManager::~SDL2AssetManager()
{
    for (auto& [key, texture] : _textures) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
    _textures.clear();
    _spritePools.clear();
    IMG_Quit();
}

SDL_Texture* SDL2AssetManager::getTexture(const std::string& key)
{
    auto it = _textures.find(key);
    if (it == _textures.end()) {
        throw std::out_of_range("Texture not loaded: " + key);
    }
    return it->second;
}

bool SDL2AssetManager::registerTexture(const std::string& file)
{
    auto logger = client::logging::Logger::getGraphicsLogger();

    if (_textures.count(file)) {
        logger->debug("Texture '{}' already registered", file);
        return true;
    }

    SDL_Surface* surface = IMG_Load(file.c_str());
    if (!surface) {
        logger->error("Failed to load texture from file: {} - {}", file, IMG_GetError());
        return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        logger->error("Failed to create texture from surface: {} - {}", file, SDL_GetError());
        return false;
    }

    _textures[file] = texture;
    logger->debug("Texture '{}' registered successfully", file);
    return true;
}

void SDL2AssetManager::removeTexture(const std::string& file)
{
    auto logger = client::logging::Logger::getGraphicsLogger();

    auto it = _textures.find(file);
    if (it != _textures.end()) {
        if (it->second) {
            SDL_DestroyTexture(it->second);
        }
        _textures.erase(it);
        _spritePools.erase(file);
        logger->debug("Texture '{}' removed", file);
    } else {
        logger->warn("Texture '{}' not found for removal", file);
    }
}

void SDL2AssetManager::addSprite(const std::string& key, float x, float y, float scaleX, float scaleY)
{
    SDL_Texture* texture = getTexture(key);

    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

    SpriteData sprite;
    sprite.texture = texture;
    sprite.destRect = {x, y, w * scaleX, h * scaleY};
    sprite.srcRect = {0, 0, w, h};
    sprite.useFullTexture = true;

    _spritePools[key].push_back(sprite);
}

void SDL2AssetManager::drawAll()
{
    for (const auto& [key, sprites] : _spritePools) {
        for (const auto& sprite : sprites) {
            SDL_RenderCopyF(_renderer, sprite.texture,
                sprite.useFullTexture ? nullptr : &sprite.srcRect,
                &sprite.destRect);
        }
    }
}
