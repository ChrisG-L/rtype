/*
 *  Errors
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

#ifndef ERRORS_HPP_
    #define ERRORS_HPP_

#include <exception>
#include <string>

namespace ECS {
    namespace ERROR {
        class UnregisteredComponent : public std::exception {
            public:
                UnregisteredComponent(const std::string& compname)
                : message("Component '" + compname + "' isn't registered!")
                {}
                ~UnregisteredComponent() {}

                const char *what() const noexcept override
                {
                    return message.c_str();
                }
            private:
                std::string message;
        };

        class InvalidEntityID : public std::exception {
            public:
                InvalidEntityID(std::size_t id)
                : message("Entity id " + std::to_string(id) + " is invalid or doesn't exist!")
                {}
                ~InvalidEntityID() {}

                const char *what() const noexcept override
                {
                    return message.c_str();
                }
            private:
                std::string message;
        };

        class ComponentNotAttached : public std::exception {
            public:
                ComponentNotAttached(std::size_t entity, const std::string& comp_name)
                : message("Entity id " + std::to_string(entity) + " doesn't have component '" + comp_name + "' attached to it!")
                {}
                ~ComponentNotAttached() {}

                const char *what() const noexcept override
                {
                    return message.c_str();
                }
            private:
                std::string message;
        };

        class ComponentAlreadyAttached : public std::exception {
            public:
                ComponentAlreadyAttached(std::size_t entity, const std::string& comp_name)
                : message("Entity id " + std::to_string(entity) + " already have component '" + comp_name + "' attached to it!")
                {}
                ~ComponentAlreadyAttached() {}

                const char *what() const noexcept override
                {
                    return message.c_str();
                }
            private:
                std::string message;
        };
    }
}

#endif /* !ERRORS_HPP_ */
