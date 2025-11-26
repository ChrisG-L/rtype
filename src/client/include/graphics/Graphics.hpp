/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Graphics
*/

#ifndef GRAPHICS_HPP_
#define GRAPHICS_HPP_

#include <iostream>

#include "utils/Vecs.hpp"

namespace graphic {
    class GraphicTexture {
        public:

            GraphicTexture(const std::string&, Vec2f pos, Vec2f scale);

            std::string getFileName() const;
            Vec2f getPos() const;
            Vec2f getScale() const;
            

        private:
            std::string _filename;
            Vec2f _pos;
            Vec2f _scale;
        };
}

#endif /* !GRAPHICS_HPP_ */
