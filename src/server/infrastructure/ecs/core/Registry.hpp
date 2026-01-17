/*
 *  Registry
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

#include <atomic>
#include <cstdint>
#include <vector>
#include <array>

#include "Component.hpp"

#ifndef REGISTRY_HPP_
    #define REGISTRY_HPP_

namespace ECS {

    class Registry {
        public:
            Registry() {}
            ~Registry() {
                for (const auto &it : m_pool_list) {
                    if (it != nullptr)
                        delete it;
                }
            }

            /**
             * @brief Checks if a component type exists in the registry.
             *
             * This function verifies whether a component pool for the given type ID is present
             * and initialized in the registry's pool list.
             *
             * @param type_id The unique identifier for the component type to check.
             * @return true if the component type exists and has an associated pool, false otherwise.
             */
            bool componentExists(uint16_t type_id) {
                return m_pool_list[type_id] != nullptr;
            }

            /**
             * @brief Checks if a component of the specified type exists in the registry.
             * 
             * This template function retrieves the type ID for the given component type T
             * and delegates to the non-template overload of componentExists(uint16_t) to
             * perform the check.
             * 
             * @tparam T The component type to check for existence. Must satisfy ComponentType.
             * @return true if a component of type T exists, false otherwise.
             */
            template <ComponentType T>
            bool componentExists() {
                uint16_t type_id = ComponentTypeId::get<T>();
                return componentExists(type_id);
            }

            /**
             * @brief Registers a new component type to the registry
             * 
             * @tparam T The type to register
             * @return true If the component has been registered successfully
             * @return false If the component was already registered
             */
            template <ComponentType T>
            bool registerComponent() {
                if (componentExists<T>())
                    return false;

                uint16_t type_id = ComponentTypeId::get<T>();
                m_pool_list[type_id] = static_cast<IComponentPool*>(new ComponentPool<T>());
                return true;
            }

            /**
             * @brief Get the Pool object
             * 
             * @tparam T The component type the pool stores
             * @return ComponentPool<T>& 
             */
            template <ComponentType T>
            ComponentPool<T> &getPool() {
                if (!componentExists<T>())
                    throw ERROR::UnregisteredComponent(typeid(T).name());
                return *(static_cast<ComponentPool<T>*>(m_pool_list[ComponentTypeId::get<T>()]));
            }

            /**
             * @brief Disables an entity from being computed
             * 
             * @param e Entity ID
             */
            void disableEntity(EntityID e) {
                for (const auto &it : m_pool_list) {
                    if (it != nullptr)
                        it->disableEntity(e);
                }
            }

        protected:
        private:
            IComponentPool *m_pool_list[UINT16_MAX]{nullptr};
    };
}

#endif /* !REGISTRY_HPP_ */
