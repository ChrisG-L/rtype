/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** IGraphic
*/

#ifndef IGRAPHIC_HPP_
#define IGRAPHIC_HPP_

#include "utils/Position.hpp"

class IGraphic {
    public:
        virtual ~IGraphic() = default;

        virtual void draw() = 0;
        
        virtual void clear() = 0;

    protected:
    private:
};

#endif /* !IGRAPHIC_HPP_ */
