/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Event
*/

#ifndef EVENT_HPP_
#define EVENT_HPP_

#include <variant>

#include "window/Closed.hpp"

using events::window::Closed;

class Event {
    private:
        std::variant<Closed> m_data;
    public:
        template<typename T>
        bool is() const {
            return std::holds_alternative<T>(m_data);
        };
};

#endif /* !EVENT_HPP_ */
