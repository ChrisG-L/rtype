/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** IGraphicPlugin
*/

#ifndef IGRAPHICPLUGIN_HPP_
#define IGRAPHICPLUGIN_HPP_

#include <memory>

#include "IWindow.hpp"

#include "utils/Vecs.hpp"


namespace graphics {
    class IGraphicPlugin {
        public:
            virtual ~IGraphicPlugin() = default;

            virtual const char* getName() const = 0;
            
            virtual std::unique_ptr<IWindow> createWindow(
                Vec2u winSize,
                const std::string& name
            ) = 0;
    };
}

extern "C" {
    graphics::IGraphicPlugin* createGraphPlugin();

    // Add destroy plugin
    const char* getPluginName();
    const char* getPluginVersion();
}

#endif /* !IGRAPHICPLUGIN_HPP_ */
