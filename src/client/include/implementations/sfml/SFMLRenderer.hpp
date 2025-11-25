/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** SFMLRenderer
*/

#ifndef SFMLRENDERER_HPP_
#define SFMLRENDERER_HPP_

#include "../../core/IRenderer.hpp"
#include "utils/AssetManager.hpp"
#include "../../graphics/Graphics.hpp"
#include <unordered_map>
#include <memory>

class SFMLRenderer: public core::IRenderer {
    public:
        SFMLRenderer(std::shared_ptr<graphics::IWindow> window);
        
        void initialize(GraphicAssets) override;
        void update() override;
        void render() override;

        void initGraphicTexture(const graphic::GraphicTexture& graphT);

    private:
        std::shared_ptr<graphics::IWindow> _window;
        std::unique_ptr<AssetManager> mAsset;
};

#endif /* !SFMLRENDERER_HPP_ */
