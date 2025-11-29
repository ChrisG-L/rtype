/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** DynamicLib
*/

#ifndef DYNAMICLIB_HPP_
#define DYNAMICLIB_HPP_

#include <dlfcn.h>

#include "../graphics/IGraphicPlugin.hpp"

namespace core {
    class DynamicLib {
        public:
            DynamicLib();
            ~DynamicLib();

            graphics::IGraphicPlugin* openGraphicLib(const std::string& libName);
            void destroyGraphicLib(graphics::IGraphicPlugin* graphLib);
        private:
            void checkDlError();

            void* _handle = nullptr;
            create_t _create_lib = nullptr;
            destroy_t _destroy_lib = nullptr;
    };
}

#endif /* !DYNAMICLIB_HPP_ */
