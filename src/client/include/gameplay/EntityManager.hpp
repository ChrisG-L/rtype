/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** EntityManager - Manages game entities lifecycle
*/

#ifndef ENTITYMANAGER_HPP_
#define ENTITYMANAGER_HPP_

#include "gameplay/GameObject.hpp"
#include "graphics/Asset.hpp"
#include <vector>
#include <memory>
#include <algorithm>

namespace gameplay {

    class EntityManager {
        public:
            EntityManager() = default;
            ~EntityManager() = default;

            EntityManager(const EntityManager&) = delete;
            EntityManager& operator=(const EntityManager&) = delete;

            template<typename T, typename... Args>
            T& spawn(Args&&... args) {
                auto entity = std::make_unique<T>(std::forward<Args>(args)...);
                T& ref = *entity;
                _entities.push_back(std::move(entity));
                return ref;
            }

            void update(float deltaTime);

            void cleanup(std::size_t& entitiesIds);

            void syncToGraphics(GraphicAssetsE& graphicElements);

            std::vector<GameObject*> getEntitiesByTag(const std::string& tag);

            size_t count() const { return _entities.size(); }
            size_t countByTag(const std::string& tag) const;

            void clear() { _entities.clear(); }

        private:
            std::vector<std::unique_ptr<GameObject>> _entities;
    };

}

#endif /* !ENTITYMANAGER_HPP_ */
