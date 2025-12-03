/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Graphics
*/

#ifndef GRAPHICS_HPP_
#define GRAPHICS_HPP_

#include <iostream>
#include <memory>
#include <iostream>

#include "utils/Vecs.hpp"
#include "Asset.hpp"

namespace graphic {
    class GraphicTexture {
        public:

            GraphicTexture(const std::string& pathName, const std::string& name);

            std::string getFileName() const;
            std::string getName() const;
            Vec2u getSize() const;

            void setSize(Vec2u size);

            graphic::GraphicTexture* findTexture(const std::string& name, GraphicAssets& assets);

        private:
            std::string _pathName;
            std::string _name;
            Vec2u _size;
        };

    enum class Layer : int {
        Background = 0,
        Entities = 10,
        Player = 20,
        Projectiles = 30,
        Effects = 40,
        UI = 100
    };

    class GraphicElement {
        public:

            GraphicElement(const GraphicTexture& texture, Vec2f pos, Vec2f scale, std::string name, Layer layer = Layer::Entities);

            const GraphicTexture& getTexture() const;
            void setTexture(const GraphicTexture& texture);

            std::string getName() const;
            Vec2f getPos() const;
            Vec2f getScale() const;
            bool isAlive() const;
            Layer getLayer() const { return _layer; }

            void setPos(Vec2f pos);
            void setScale(Vec2f scale);
            void setLayer(Layer layer) { _layer = layer; }
            void setDie();

            graphic::GraphicElement* findElement(const std::string& name, GraphicAssetsE& elements);


        private:
            GraphicTexture _texture;
            Vec2f _pos;
            Vec2f _scale;
            std::string _name;
            bool _isAlive;
            Layer _layer;
        };
}

#endif /* !GRAPHICS_HPP_ */
