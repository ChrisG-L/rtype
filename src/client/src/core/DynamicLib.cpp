/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** DynamicLib
*/

#include "core/DynamicLib.hpp"
#include "graphics/IGraphicPlugin.hpp"
#include <iostream>
#include <stdexcept>

namespace core {
    DynamicLib::DynamicLib() {}

    DynamicLib::~DynamicLib()
    {
        if (_handle) {
            dlclose(_handle);
            _handle = nullptr;
        }
    }

    graphics::IGraphicPlugin* DynamicLib::openGraphicLib(const std::string& libName) {
        dlerror();

        _handle = dlopen(libName.c_str(), RTLD_LAZY);
        if (!_handle) {
            const char* error = dlerror();
            throw std::runtime_error(std::string("DLOPEN: ") + (error ? error : "unknown error"));
        }

        dlerror();
        _create_lib = reinterpret_cast<create_t>(dlsym(_handle, "create"));
        checkDlError();

        return _create_lib();
    }

    void DynamicLib::destroyGraphicLib(graphics::IGraphicPlugin* graphLib) {
        dlerror();
        _destroy_lib = reinterpret_cast<destroy_t>(dlsym(_handle, "destroy"));
        checkDlError();
        _destroy_lib(graphLib);
    }

    void DynamicLib::checkDlError() {
        const char* error = dlerror();
        if (error) {
            std::cerr << "Erreur dlsym: " << error << std::endl;
            throw std::runtime_error(std::string("DLSYM: ") + error);
        }
    }
}
