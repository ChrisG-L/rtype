/*
 *  System
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

#ifndef SYSTEM_HPP_
    #define SYSTEM_HPP_

#include "ECS.hpp"

namespace ECS {

    /**
     * @brief System Interface, every system should inherit from this class
     */
    class ISystem {
        public:
            virtual ~ISystem() = default;

            virtual void Update(ECS &, SystemID thisID, uint32_t msecs) = 0;
        protected:
        private:
    };
}
#endif /* !SYSTEM_HPP_ */
