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

#ifdef _WIN32
    #include <windows.h>
#endif

namespace core {
    DynamicLib::DynamicLib() {}

    DynamicLib::~DynamicLib()
    {
        if (_handle) {
            #ifdef _WIN32
                FreeLibrary(_handle);
            #else
                dlclose(_handle);
            #endif
            _handle = nullptr;
        }
    }

    graphics::IGraphicPlugin* DynamicLib::openGraphicLib(const std::string& libName) {
        #ifdef _WIN32
            _handle = LoadLibraryA(libName.c_str());
            if (!_handle) {
                DWORD error = GetLastError();
                throw std::runtime_error("LoadLibrary failed with error code: " + std::to_string(error));
            }

            _create_lib = reinterpret_cast<create_t>(GetProcAddress(_handle, "create"));
            if (!_create_lib) {
                DWORD error = GetLastError();
                throw std::runtime_error("GetProcAddress(create) failed with error code: " + std::to_string(error));
            }
        #else
            dlerror();

            _handle = dlopen(libName.c_str(), RTLD_LAZY);
            if (!_handle) {
                const char* error = dlerror();
                throw std::runtime_error(std::string("DLOPEN: ") + (error ? error : "unknown error"));
            }

            dlerror();
            _create_lib = reinterpret_cast<create_t>(dlsym(_handle, "create"));
            checkError();
        #endif

        return _create_lib();
    }

    void DynamicLib::destroyGraphicLib(graphics::IGraphicPlugin* graphLib) {
        #ifdef _WIN32
            _destroy_lib = reinterpret_cast<destroy_t>(GetProcAddress(_handle, "destroy"));
            if (!_destroy_lib) {
                DWORD error = GetLastError();
                throw std::runtime_error("GetProcAddress(destroy) failed with error code: " + std::to_string(error));
            }
        #else
            dlerror();
            _destroy_lib = reinterpret_cast<destroy_t>(dlsym(_handle, "destroy"));
            checkError();
        #endif
        _destroy_lib(graphLib);
    }

    void DynamicLib::checkError() {
        #ifdef _WIN32
            DWORD error = GetLastError();
            if (error != 0) {
                std::cerr << "Windows error: " << error << std::endl;
                throw std::runtime_error("Windows error code: " + std::to_string(error));
            }
        #else
            const char* error = dlerror();
            if (error) {
                std::cerr << "Erreur dlsym: " << error << std::endl;
                throw std::runtime_error(std::string("DLSYM: ") + error);
            }
        #endif
    }
}
