/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** RenderSystem - Système de rendu
*/

#pragma once

#include "../Entity.hpp"
#include "../System.hpp"
#include "../Registry.hpp"
#include "../components/Transform.hpp"
#include "../components/Sprite.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace rtype::ecs::systems {

/**
 * @brief Donnée de rendu pour le tri par z-order
 */
struct RenderData {
    Entity entity;
    int zOrder;
    components::Transform transform;
    components::Sprite sprite;
};

/**
 * @brief Système de rendu des sprites
 *
 * Récupère toutes les entités visibles, les trie par z-order,
 * et les dessine sur la cible de rendu.
 */
class RenderSystem : public ISystem {
public:
    RenderSystem() = default;

    void update(Registry& /*registry*/, float /*dt*/) override {
        // Note: Le rendu est fait dans une méthode séparée
        // car il nécessite la RenderTarget
    }

    /**
     * @brief Effectue le rendu sur une cible
     */
    void render(Registry& registry, sf::RenderTarget& target) {
        m_renderQueue.clear();

        // Collecte les entités à dessiner
        registry.view<components::Transform, components::Sprite>(
            [this](Entity entity, components::Transform& transform, components::Sprite& sprite) {
                if (!sprite.visible) return;

                m_renderQueue.push_back({
                    entity,
                    sprite.zOrder,
                    transform,
                    sprite
                });
            });

        // Trie par z-order (les plus bas sont dessinés en premier)
        std::sort(m_renderQueue.begin(), m_renderQueue.end(),
            [](const RenderData& a, const RenderData& b) {
                return a.zOrder < b.zOrder;
            });

        // Dessine les sprites
        for (const auto& data : m_renderQueue) {
            drawSprite(target, data);
        }
    }

    [[nodiscard]] int priority() const override {
        return 100; // Dernier système (mais le rendu se fait séparément)
    }

    /**
     * @brief Ajoute une texture au cache
     */
    void addTexture(const std::string& id, const sf::Texture& texture) {
        m_textures[id] = texture;
    }

    /**
     * @brief Récupère une texture du cache
     */
    [[nodiscard]] sf::Texture* getTexture(const std::string& id) {
        auto it = m_textures.find(id);
        if (it == m_textures.end()) return nullptr;
        return &it->second;
    }

private:
    void drawSprite(sf::RenderTarget& target, const RenderData& data) {
        // Récupère la texture
        auto* texture = getTexture(data.sprite.textureId);

        if (!texture) {
            // Fallback: dessine un rectangle coloré si pas de texture
            sf::RectangleShape rect;

            // Utilise la taille du textureRect ou une taille par défaut
            float width = (data.sprite.textureRect.size.x > 0)
                ? static_cast<float>(data.sprite.textureRect.size.x)
                : 32.0f;
            float height = (data.sprite.textureRect.size.y > 0)
                ? static_cast<float>(data.sprite.textureRect.size.y)
                : 32.0f;

            rect.setSize({width, height});
            rect.setFillColor(data.sprite.color);
            rect.setPosition({data.transform.x, data.transform.y});
            rect.setRotation(sf::degrees(data.transform.rotation));
            rect.setScale({data.transform.scaleX, data.transform.scaleY});

            target.draw(rect);
            return;
        }

        // Configure le sprite SFML
        sf::Sprite sfSprite(*texture);

        // Applique le rectangle de texture si défini
        if (data.sprite.textureRect.size.x > 0 && data.sprite.textureRect.size.y > 0) {
            sfSprite.setTextureRect(data.sprite.textureRect);
        }

        // Applique la transformation
        sfSprite.setPosition({data.transform.x, data.transform.y});
        sfSprite.setRotation(sf::degrees(data.transform.rotation));
        sfSprite.setScale({data.transform.scaleX, data.transform.scaleY});
        sfSprite.setColor(data.sprite.color);

        // Dessine
        target.draw(sfSprite);
    }

private:
    std::vector<RenderData> m_renderQueue;
    std::unordered_map<std::string, sf::Texture> m_textures;
};

} // namespace rtype::ecs::systems
