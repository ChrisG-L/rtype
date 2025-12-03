/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** AssetManager
*/

#ifndef ASSETMANAGER_HPP_
#define ASSETMANAGER_HPP_

#include <SFML/Graphics.hpp>

#include <iostream>
#include <memory>
#include <unordered_map>
#include <map>

#include "graphics/IWindow.hpp"
#include "graphics/Graphics.hpp"
#include "core/Logger.hpp"

struct SpriteData {
    sf::Sprite sprite;
    int zIndex = 0;
};

class AssetManager {
    public:
        AssetManager();

        sf::Texture& getTexture(const std::string& key);
        sf::Sprite& getSprite(const std::string& textureKey, const std::string& spriteKey);

        bool registerTexture(graphic::GraphicTexture&, const std::string& file);
        void removeTexture(const std::string& file);

        void addSprite(const std::string& textureKey, const std::string& spriteKey, const sf::Sprite& sprite, int zIndex = 0);
        void setSprite(const std::string& textureKey, const std::string& spriteKey, const sf::Sprite& sprite, int zIndex = 0);
        void removeSprite(const std::string& textureKey, const std::string& spriteKey);
        void drawAll(sf::RenderWindow* window);

    private:
        std::unordered_map<std::string, sf::Texture> textures;
        std::unordered_map<std::string, std::unordered_map<std::string, SpriteData>> spritePools;
};

#endif /* !ASSETMANAGER_HPP_ */
