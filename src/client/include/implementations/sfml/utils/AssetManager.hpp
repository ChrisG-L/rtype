/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** AssetManager
*/

#ifndef ASSETMANAGER_HPP_
#define ASSETMANAGER_HPP_

#include <SFML/Graphics.hpp>
#include "../../../graphics/IWindow.hpp"

#include <unordered_map>
#include <iostream>


class AssetManager {
    public:
        AssetManager() {};

        sf::Texture& getTexture(const std::string& key) {
            auto textIt = textures.find(key);
            if (textIt == textures.end())
                throw std::out_of_range("Texture non chargée.");
            return textIt->second;
        }

        void registerTexture(const std::string& file) {
            if (textures.count(file))
                return;

            sf::Texture newTexture;
            if (!newTexture.loadFromFile(file)) {
                std::cout << "La texture n'a pas pu etre chargé correctement!" << std::endl;
                return;
            }
            textures.emplace(file, std::move(newTexture));
        }
        
        void removeTexture(const std::string& file) {
            if (textures.count(file))
                textures.erase(file);
            else
                std::cerr << "Erreur : Texture '" << file << "' non trouvée pour suppression." << std::endl;
        }

        void addSprite(const std::string& key, const sf::Sprite& sprite) {
            spritePools[key].push_back(sprite);
        }

        void drawAll(graphics::IWindow* window) {
            for (auto const& [key, sprites]: spritePools) {
                for (const auto& sprite: sprites) {
                    window->draw(sprite);
                }
            }
        }

    private:
        std::unordered_map<std::string, sf::Texture> textures;

        std::unordered_map<std::string, std::vector<sf::Sprite>> spritePools;

    protected:
    private:
};

#endif /* !ASSETMANAGER_HPP_ */
