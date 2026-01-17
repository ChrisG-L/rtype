/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Event
*/

#ifndef EVENT_HPP_
#define EVENT_HPP_

#include <variant>
#include <optional>
#include <cstdint>

namespace events {

    enum class Key {
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Num0, Num1, Num2, Num3, Num4, Num5,
        Num6, Num7, Num8, Num9,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        Space, Enter, Escape, Tab, Backspace,
        Up, Down, Left, Right,
        LShift, RShift, LCtrl, RCtrl, LAlt, RAlt,
        Unknown
    };

    enum class MouseButton {
        Left,
        Right,
        Middle,
        Unknown
    };

    struct KeyPressed {
        Key key;
    };

    struct KeyReleased {
        Key key;
    };

    struct MouseMoved {
        int x;
        int y;
    };

    struct MouseButtonPressed {
        MouseButton button;
        int x;
        int y;
    };

    struct MouseButtonReleased {
        MouseButton button;
        int x;
        int y;
    };

    struct TextEntered {
        uint32_t unicode;
    };

    struct WindowClosed {};

    struct None {};

    using Event = std::variant<
        None,
        KeyPressed,
        KeyReleased,
        MouseMoved,
        MouseButtonPressed,
        MouseButtonReleased,
        TextEntered,
        WindowClosed
    >;
    
}

#endif /* !EVENT_HPP_ */
