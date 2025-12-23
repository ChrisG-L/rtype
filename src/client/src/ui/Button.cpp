/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Button
*/

#include "ui/Button.hpp"
#include <variant>

namespace ui {

Button::Button(const Vec2f& pos, const Vec2f& size, const std::string& text,
               const std::string& fontKey)
    : _pos(pos)
    , _size(size)
    , _text(text)
    , _fontKey(fontKey)
    , _state(State::Normal)
    , _focused(false)
{
}

void Button::handleEvent(const events::Event& event)
{
    if (!_enabled) {
        _state = State::Disabled;
        return;
    }

    if (auto* moved = std::get_if<events::MouseMoved>(&event)) {
        bool isHovered = contains(static_cast<float>(moved->x), static_cast<float>(moved->y));

        if (_state == State::Pressed) {
            // Keep pressed state while mouse button is down
        } else if (isHovered) {
            _state = State::Hovered;
        } else {
            _state = State::Normal;
        }
    }

    if (auto* pressed = std::get_if<events::MouseButtonPressed>(&event)) {
        if (pressed->button == events::MouseButton::Left &&
            contains(static_cast<float>(pressed->x), static_cast<float>(pressed->y))) {
            _state = State::Pressed;
        }
    }

    if (auto* released = std::get_if<events::MouseButtonReleased>(&event)) {
        if (released->button == events::MouseButton::Left && _state == State::Pressed) {
            if (contains(static_cast<float>(released->x), static_cast<float>(released->y))) {
                _state = State::Hovered;
                if (_onClick) {
                    _onClick();
                    return;  // Callback may have destroyed 'this'
                }
            } else {
                _state = State::Normal;
            }
        }
    }

    // Keyboard support: Enter to click when focused
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        if (_focused && keyPressed->key == events::Key::Enter) {
            if (_onClick) {
                _onClick();
                return;  // Callback may have destroyed 'this'
            }
        }
    }
}

void Button::update(float deltaTime)
{
    (void)deltaTime;
}

void Button::render(graphics::IWindow& window)
{
    if (!_visible) return;

    rgba bgColor;
    switch (_state) {
        case State::Normal:
            bgColor = _normalColor;
            break;
        case State::Hovered:
            bgColor = _hoveredColor;
            break;
        case State::Pressed:
            bgColor = _pressedColor;
            break;
        case State::Disabled:
            bgColor = _disabledColor;
            break;
    }

    // Draw background
    window.drawRect(_pos.x, _pos.y, _size.x, _size.y, bgColor);

    // Draw border (2px)
    window.drawRect(_pos.x, _pos.y, _size.x, 2, _borderColor);                          // Top
    window.drawRect(_pos.x, _pos.y + _size.y - 2, _size.x, 2, _borderColor);            // Bottom
    window.drawRect(_pos.x, _pos.y, 2, _size.y, _borderColor);                          // Left
    window.drawRect(_pos.x + _size.x - 2, _pos.y, 2, _size.y, _borderColor);            // Right

    // Draw focus indicator
    if (_focused) {
        rgba focusColor{100, 150, 255, 255};
        window.drawRect(_pos.x - 2, _pos.y - 2, _size.x + 4, 2, focusColor);
        window.drawRect(_pos.x - 2, _pos.y + _size.y, _size.x + 4, 2, focusColor);
        window.drawRect(_pos.x - 2, _pos.y, 2, _size.y, focusColor);
        window.drawRect(_pos.x + _size.x, _pos.y, 2, _size.y, focusColor);
    }

    // Draw centered text
    unsigned int fontSize = 20;
    float textWidth = static_cast<float>(_text.length()) * 10.0f;  // Approximate
    float textX = _pos.x + (_size.x - textWidth) / 2.0f;
    float textY = _pos.y + (_size.y - fontSize) / 2.0f;

    rgba textColor = _state == State::Disabled ? rgba{128, 128, 128, 255} : _textColor;
    window.drawText(_fontKey, _text, textX, textY, fontSize, textColor);
}

} // namespace ui
