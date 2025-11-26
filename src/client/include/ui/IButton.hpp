/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** IButton
*/

#ifndef IBUTTON_HPP_
#define IBUTTON_HPP_


#include <string>

#include "events/Signal.hpp"

namespace ui {
    class IButton {
        public:
            Signal<std::string, int> onClick;

            virtual ~IButton() = default;


            virtual void click() = 0;
    };
};

#endif /* !IBUTTON_HPP_ */
