/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SDL2Plugin
*/

#include "plugins/SDL2Plugin.hpp"

namespace sdl2 {
    SDL2Plugin::SDL2Plugin() {}

    const char* SDL2Plugin::getName() const {
        return "SDL2 Graphic";
    }

    std::shared_ptr<graphics::IWindow> SDL2Plugin::createWindow(
        Vec2u winSize,
        const std::string& name
    ) {
        return std::make_shared<SDL2Window>(winSize, name);
    }

    std::shared_ptr<core::IRenderer> SDL2Plugin::createRenderer(
        std::shared_ptr<graphics::IWindow> window
    ) {
        return std::make_shared<SDL2Renderer>(window);
    }
}

extern "C" {
    graphics::IGraphicPlugin* create() {
        return new sdl2::SDL2Plugin();
    }

    void destroy(graphics::IGraphicPlugin* graphPlugin) {
        delete graphPlugin;
    }

    const char* getPluginName() {
        return "SDL2 Plugin";
    }

    const char* getPluginVersion() {
        return "1.0.0";
    }
}
