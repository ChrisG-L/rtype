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
#include "core/IRenderer.hpp"
#include "utils/Vecs.hpp"


namespace graphics {
    class IGraphicPlugin {
        public:
            virtual ~IGraphicPlugin() = default;

            virtual const char* getName() const = 0;
            
            virtual std::shared_ptr<IWindow> createWindow(
                Vec2u winSize,
                const std::string& name
            ) = 0;

            virtual std::shared_ptr<core::IRenderer> createRenderer(
                std::shared_ptr<graphics::IWindow> window
            ) = 0;
    };
}

typedef graphics::IGraphicPlugin* (*create_t)();
typedef void (*destroy_t)(graphics::IGraphicPlugin*);

#endif /* !IGRAPHICPLUGIN_HPP_ */
