/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** ComponentManager
*/

#ifndef COMPONENTMANAGER_HPP_
#define COMPONENTMANAGER_HPP_

#include "../ecs/Types.hpp"
#include "IComponent.hpp"

#include <unordered_map>
#include <array>
#include <vector>
#include <memory>
#include <typeindex>
#include <any>

class ComponentManager {
    public:
        ComponentManager(): mComponentEntities{} {};

        template<typename T>
        void registerComponent() {
            std::type_index typeId = typeid(T);
            if (mComponentEntities.find(typeId) == mComponentEntities.end())
                mComponentEntities[typeId] = std::unordered_map<Entity, std::any>();
        }

        template<typename T>
        void addComponent(Entity entity, T component) {
            std::type_index typeId = typeid(T);
            if (mComponentEntities.find(typeId) != mComponentEntities.end())
                mComponentEntities[typeId][entity] = component;
        }

        template<typename T>
        T* getComponent(Entity entity) {
            std::type_index typeId = typeid(T);

            auto typeIt = mComponentEntities.find(typeId);
            if (typeIt == mComponentEntities.end())
                return nullptr;
            
            auto entityIt = mComponentEntities[typeId].find(entity);
            if (entityIt == mComponentEntities[typeId].end())
                return nullptr;
            
            return &std::any_cast<T&>(entityIt->second);
        }
        
        template<typename T>
        bool hasComponent(Entity entity) {
            std::type_index typeId = typeid(T);

            auto typeIt = mComponentEntities.find(typeId);
            if (typeIt == mComponentEntities.end())
                return false;

            auto entityIt = mComponentEntities[typeId].find(entity);
            if (entityIt == mComponentEntities[typeId].end())
                return false;
            return true;
        }

        template<typename... ComponentTypes>
        std::vector<Entity> getEntitiesWithComponents() {
            std::vector<Entity> result;

            for (Entity entity = 0; entity < MAX_ENTITIES; entity++) {
                if ((hasComponent<ComponentTypes>(entity) && ...)) {
                    result.push_back(entity);
                }
            }
            return result;
        }

    private:
        std::unordered_map<std::type_index, std::unordered_map<Entity, std::any>> mComponentEntities;

};

#endif /* !COMPONENTMANAGER_HPP_ */
