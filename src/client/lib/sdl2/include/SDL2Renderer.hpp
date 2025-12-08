/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** SDL2Renderer
*/

#ifndef SDL2RENDERER_HPP_
#define SDL2RENDERER_HPP_

#include <SDL2/SDL.h>

#include <memory>
#include <stdexcept>

#include "core/IRenderer.hpp"
#include "utils/SDL2AssetManager.hpp"
#include "graphics/Graphics.hpp"
#include "graphics/IWindow.hpp"
#include "utils/Vecs.hpp"

class SDL2Renderer: public core::IRenderer {
    public:
        SDL2Renderer(std::shared_ptr<graphics::IWindow> window);

        void initialize(GraphicAssets& assets, GraphicAssetsE& elements) override;
        void update(float deltatime, GraphicAssets& assets, GraphicAssetsE& elements) override;
        void render() override;

        void initGraphicTexture(const graphic::GraphicTexture& graphT);
        void initGraphicElement(const graphic::GraphicElement& elem);

    private:
        std::shared_ptr<graphics::IWindow> _window;
        SDL_Renderer* _sdlRenderer;
        std::unique_ptr<SDL2AssetManager> _assetManager;
        GraphicAssetsE* _elements = nullptr;
};

#endif /* !SDL2RENDERER_HPP_ */
