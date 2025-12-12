/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SDL2AssetManager
*/

#ifndef SDL2ASSETMANAGER_HPP_
#define SDL2ASSETMANAGER_HPP_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

#include "graphics/IWindow.hpp"
#include "core/Logger.hpp"

struct SpriteData {
    SDL_Texture* texture;
    SDL_FRect destRect;
    SDL_Rect srcRect;
    bool useFullTexture;
};

class SDL2AssetManager {
    public:
        SDL2AssetManager(SDL_Renderer* renderer);
        ~SDL2AssetManager();

        SDL_Texture* getTexture(const std::string& key);
        bool registerTexture(const std::string& file);
        void removeTexture(const std::string& file);
        void addSprite(const std::string& key, float x, float y, float scaleX, float scaleY);
        void drawAll();

    private:
        SDL_Renderer* _renderer;
        std::unordered_map<std::string, SDL_Texture*> _textures;
        std::unordered_map<std::string, std::vector<SpriteData>> _spritePools;
};

#endif /* !SDL2ASSETMANAGER_HPP_ */
