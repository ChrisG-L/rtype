/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SDL2Plugin
*/

#ifndef SDL2PLUGIN_HPP_
#define SDL2PLUGIN_HPP_

#include <memory>

#include "graphics/IGraphicPlugin.hpp"
#include "../SDL2Window.hpp"
#include "../SDL2Renderer.hpp"

namespace sdl2 {
    class SDL2Plugin: public graphics::IGraphicPlugin {
        public:
            SDL2Plugin();

            const char* getName() const override;

            std::shared_ptr<graphics::IWindow> createWindow(
                Vec2u winSize,
                const std::string& name
            ) override;

            std::shared_ptr<core::IRenderer> createRenderer(
                std::shared_ptr<graphics::IWindow> window
            ) override;

        protected:
        private:
    };
}

#endif /* !SDL2PLUGIN_HPP_ */
