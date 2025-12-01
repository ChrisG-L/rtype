/*
 *  Component
 *
 *  Blob ECS is a lightweight Entity Component System library
 *  Copyright (C) 2025 LECOCQ Guillaume
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
*/

#ifndef COMPONENT_HPP_
    #define COMPONENT_HPP_

#include "Includes.hpp"
#include "Errors.hpp"
#include <vector>
#include <limits>
#include <cstdint>
#include <iostream>

namespace ECS {

    /**
     * @brief Class for generating and getting custom type IDs
     */
    class ComponentTypeId {
        private:
            inline static std::atomic<uint16_t> runtime_counter = 0;
        public:
            /**
             * @brief Returns the ID attached to this specific type, generates an ID if this is the first call
             * 
             * @tparam T The type to get an ID from
             * @return uint16_t The corresponding ID
             */
            template<typename T>
            static uint16_t get() {
                static const uint16_t id = runtime_counter.fetch_add(1, std::memory_order_relaxed);
                return id;
            }
    };

    /**
     * @brief A cell that stores an entity ID with its attached component
     * 
     * @tparam T The component type
     */
    template<typename T>
    struct DenseComponent {
        T component;
        EntityID entity;
    };

    /**
     * @brief Structure for a custom sparse set
     * 
     * @tparam T The type of the set
     */
    template<ComponentType T>
    struct SparseSetData {
        std::vector<DenseComponent<T>> dense_components;           // Packed components
        std::vector<uint32_t> sparse;                           // EntityID -> dense index mapping
    };
    constexpr uint32_t NULL_INDEX = std::numeric_limits<uint32_t>::max();

    /**
     * @brief ComponentPool interface, this should be casted to a <compType>ComponentPool to be used
     */
    class IComponentPool {
        public:
            virtual ~IComponentPool() = default;
            virtual void disableEntity(EntityID) = 0;
    };

    /**
     * @brief ComponentPool that stores components of the defined type
     * 
     * @tparam T The type of component the pool is storing
     */
    template <ComponentType T>
    class ComponentPool : public IComponentPool {
        public:
            ComponentPool() : m_cache_dirty(true) {
                m_data.dense_components.reserve(10000);
                m_data.sparse.reserve(100000);
            }

            ~ComponentPool() {
            }

            /**
             * @brief Checks if a specific entity has the component
             * 
             * @param e The entity ID
             * @return true The entity has the component
             * @return false The entity does not have the component
             */
            bool hasComponent(EntityID e) {
                return e < m_data.sparse.size() && m_data.sparse[e] != NULL_INDEX;
            }

            /**
             * @brief Adds the component to the specified entity
             * 
             * @param e The entity ID
             * @return T& Reference to the newly created component
             */
            T &addComponent(EntityID e) {
                if (hasComponent(e))
                    throw ERROR::ComponentAlreadyAttached(e, std::string(typeid(T).name()));

                if (e >= m_data.sparse.size())
                    sparseGrow(e);

                uint32_t new_dense_index = static_cast<uint32_t>(m_data.dense_components.size());
                m_data.dense_components.emplace_back();

                m_data.dense_components[new_dense_index].entity = e;

                m_data.sparse[e] = new_dense_index;

                m_cache_dirty = true;
                return m_data.dense_components.back().component;
            }

            /**
             * @brief Removes the component attached to the specified entity
             * 
             * @param e The entity ID
             */
            void removeComponent(EntityID e) {
                if (!hasComponent(e)) return;

                uint32_t dense_index = m_data.sparse[e];
                uint32_t last_index = static_cast<uint32_t>(m_data.dense_components.size() - 1);

                if (dense_index != last_index) {
                    m_data.dense_components[dense_index] = std::move(m_data.dense_components[last_index]);
                    
                    EntityID moved_entity = m_data.dense_components[dense_index].entity;
                    m_data.sparse[moved_entity] = dense_index;
                }

                m_data.dense_components.pop_back();

                m_data.sparse[e] = NULL_INDEX;

                m_cache_dirty = true;
            }

            /**
             * @brief Get the Component object attached to the specified entity
             * 
             * @param e The entity ID
             * @return T& Reference to the component
             */
            T &getComponent(EntityID e) {
                
                return m_data.dense_components[m_data.sparse[e]].component;
            }

            /**
             * @brief Get a vector containing every entity's id which have this component attached
             * 
             * @return const std::vector<EntityID>& Vector of entities
             */
            const std::vector<EntityID> &getActiveEntities() {
                if (m_cache_dirty) {
                    m_cached_entities = m_data.dense_components;
                    std::sort(m_cached_entities.begin(), m_cached_entities.end());
                    m_cache_dirty = false;
                }
                return m_cached_entities;
            }

            /**
             * @brief Alias for removeComponent(e)
             * 
             * @param e The entity ID
             */
            void disableEntity(EntityID e) override {
                removeComponent(e);
            }

            /**
             * @brief Get the Pool object
             * 
             * @return const std::vector<EntityID>& 
             */
            const std::vector<EntityID>& getPool() {
                return getActiveEntities();
            }

        private:
            SparseSetData<T> m_data;
            std::vector<EntityID> m_cached_entities;
            bool m_cache_dirty;

            void sparseGrow(EntityID new_standard) {
                std::size_t curr_size = m_data.sparse.size();
                if (curr_size == 0)
                    curr_size = 8192;
                else
                    while (curr_size <= new_standard) curr_size <<= 1;
                m_data.sparse.resize(curr_size, NULL_INDEX);
            }
    };
}

#endif /* !COMPONENT_HPP_ */
