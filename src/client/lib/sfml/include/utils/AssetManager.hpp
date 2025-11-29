/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** AssetManager
*/

#ifndef ASSETMANAGER_HPP_
#define ASSETMANAGER_HPP_

#include <SFML/Graphics.hpp>

#include <memory>
#include <unordered_map>

#include "graphics/IWindow.hpp"
#include "core/Logger.hpp"

class AssetManager {
    public:
        AssetManager();

        sf::Texture& getTexture(const std::string& key);
        bool registerTexture(const std::string& file);
        void removeTexture(const std::string& file);
        void addSprite(const std::string& key, const sf::Sprite& sprite);
        void drawAll(sf::RenderWindow* window);

    private:
        std::unordered_map<std::string, sf::Texture> textures;
        std::unordered_map<std::string, std::vector<sf::Sprite>> spritePools;
};

#endif /* !ASSETMANAGER_HPP_ */
