/*
 *  Includes
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

#ifndef INCLUDES_HPP_
    #define INCLUDES_HPP_

    #include <cstddef>
    #include <concepts>
    #include <type_traits>
    #include <typeindex>
    #include <cstdint>
    #include <atomic>

namespace ECS {
    class ISystem;
    class ECS;
        
    // Concept for accepting system classes
    template<typename T>
    concept SystemClass = std::is_base_of_v<ISystem, T> && std::is_constructible_v<T>;

    // Concept for accepting components
    template<typename T>
    concept ComponentType = std::is_default_constructible_v<T>;

    // Alias for uint32_t, used to represent, locate and perform actions on entities
    using EntityID = uint32_t;

    // Alias for uint16_t, used to represent a System within the ECS
    using SystemID = uint16_t;

    // Entity groups, modify this enum to add new groups to the system
    enum EntityGroup {
        NONE,
        EXAMPLES
    };

    /*
        This is how an entity is stored within the ECS
        isActive represents if an entity exists or not
        group represents a group which the entity belongs to
    */
    struct Entity {
        bool isActive = false;
        EntityGroup group = NONE;
    };

    struct SystemData {
        bool enabled;
        ISystem *sys;
        int tickrate;
        int skipped_ticks;
    };
}

#endif /* !INCLUDES_HPP_ */
