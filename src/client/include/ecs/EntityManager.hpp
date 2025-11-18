/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** EntityManager
*/

#ifndef ENTITYMANAGER_HPP_
#define ENTITYMANAGER_HPP_

#include "Types.hpp"
#include <queue>

class EntityManager {
    public:
        EntityManager(): entities{} {
            for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
                entities.push(entity);
            }
        };

        Entity createEntity() {
            Entity id = entities.front();
            entities.pop();
            return id;
        }

        void destroyEntity(Entity entity) {
            entities.push(entity);
        }

    protected:
    private:
        std::queue<Entity> entities;
};

#endif /* !ENTITYMANAGER_HPP_ */
