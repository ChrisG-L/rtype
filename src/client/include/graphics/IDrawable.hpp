/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** IDrawable
*/

#ifndef IDRAWABLE_HPP_
#define IDRAWABLE_HPP_

#include <SFML/Graphics.hpp>

namespace graphics {
    class IDrawable {
        public:
            virtual ~IDrawable() = default;

            virtual void draw(sf::RenderWindow& window) = 0;
        protected:
        private:
    };
}

#endif /* !IDRAWABLE_HPP_ */
