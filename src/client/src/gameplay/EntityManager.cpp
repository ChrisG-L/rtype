/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** EntityManager
*/

#include "gameplay/EntityManager.hpp"

namespace gameplay {

    void EntityManager::update(float deltaTime)
    {
        for (auto& entity : _entities) {
            if (entity && entity->isAlive()) {
                entity->update(deltaTime);
            }
        }
    }

    void EntityManager::cleanup(std::size_t& entitiesIds)
    {
        _entities.erase(
            std::remove_if(_entities.begin(), _entities.end(),
                [&entitiesIds](const std::unique_ptr<GameObject>& entity) {
                    if (!entity)
                        return false;
                    if (!entity->isAlive() && entitiesIds > 0)
                        entitiesIds--;
                    return !entity->isAlive();
                }),
            _entities.end()
        );
    }

    void EntityManager::syncToGraphics(GraphicAssetsE& graphicElements)
    {
        graphicElements.erase(
            std::remove_if(graphicElements.begin(), graphicElements.end(),
                [](const AssetE& asset) {
                    const auto& elem = std::get<graphic::GraphicElement>(asset);
                    return elem.getName().find("missile") != std::string::npos;
                }),
            graphicElements.end()
        );

        for (auto& entity : _entities) {
            if (entity && entity->isAlive()) {
                graphicElements.push_back(entity->getGraphicElement());
            }
        }
    }

    std::vector<GameObject*> EntityManager::getEntitiesByTag(const std::string& tag)
    {
        std::vector<GameObject*> result;
        for (auto& entity : _entities) {
            if (entity && entity->isAlive() && entity->getTag() == tag) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    size_t EntityManager::countByTag(const std::string& tag) const
    {
        return std::count_if(_entities.begin(), _entities.end(),
            [&tag](const std::unique_ptr<GameObject>& entity) {
                return entity && entity->isAlive() && entity->getTag() == tag;
            });
    }

}
