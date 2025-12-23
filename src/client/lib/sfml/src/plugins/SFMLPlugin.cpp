/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SFMLPlugin
*/


#include "plugins/SFMLPlugin.hpp"

namespace sfml {
    SFMLPlugin::SFMLPlugin() {};

    const char* SFMLPlugin::getName() const {
        return "SFML Graphic";
    }

    std::shared_ptr<graphics::IWindow> SFMLPlugin::createWindow(
        Vec2u winSize,
        const std::string& name
    ) {
        return std::make_shared<SFMLWindow>(winSize, name);
    }

    std::shared_ptr<core::IRenderer> SFMLPlugin::createRenderer(
        std::shared_ptr<graphics::IWindow> window
    ) {
        return std::make_shared<SFMLRenderer>(window);
    }
}


extern "C" {
    graphics::IGraphicPlugin* create() {
        return new sfml::SFMLPlugin();
    };

    void destroy(graphics::IGraphicPlugin* graphPlugin) {
        delete(graphPlugin);
    };

    // Add destroy plugin
    const char* getPluginName() {
        return "SFML Plugin";
    };

    const char* getPluginVersion() {
        return "1.0.0";
    };
}
