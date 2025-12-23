/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** TextInput
*/

#include "ui/TextInput.hpp"
#include <variant>
#include <algorithm>

namespace ui {

TextInput::TextInput(const Vec2f& pos, const Vec2f& size, const std::string& placeholder,
                     const std::string& fontKey, bool isPassword)
    : _pos(pos)
    , _size(size)
    , _placeholder(placeholder)
    , _fontKey(fontKey)
    , _isPassword(isPassword)
    , _cursorPos(0)
{
}

void TextInput::setText(const std::string& text)
{
    _text = text.substr(0, _maxLength);
    _cursorPos = _text.length();
    if (_onChange) {
        _onChange(_text);
    }
}

void TextInput::clear()
{
    _text.clear();
    _cursorPos = 0;
    if (_onChange) {
        _onChange(_text);
    }
}

void TextInput::setFocused(bool focused)
{
    _focused = focused;
    if (focused) {
        _showCursor = true;
        _cursorBlinkTimer = 0.0f;
    }
}

void TextInput::handleEvent(const events::Event& event)
{
    if (!_enabled) return;

    // Click to focus
    if (auto* pressed = std::get_if<events::MouseButtonPressed>(&event)) {
        if (pressed->button == events::MouseButton::Left) {
            bool wasClicked = contains(static_cast<float>(pressed->x), static_cast<float>(pressed->y));
            setFocused(wasClicked);

            if (wasClicked) {
                // Position cursor based on click position
                float relativeX = static_cast<float>(pressed->x) - _pos.x - 10.0f;
                size_t clickPos = static_cast<size_t>(relativeX / DEFAULT_CHAR_WIDTH);
                _cursorPos = std::min(clickPos, _text.length());
            }
        }
    }

    if (!_focused) return;

    // Text input
    if (auto* textEvent = std::get_if<events::TextEntered>(&event)) {
        uint32_t unicode = textEvent->unicode;
        // Printable ASCII characters (32-126)
        if (unicode >= 32 && unicode < 127 && _text.length() < _maxLength) {
            _text.insert(_cursorPos, 1, static_cast<char>(unicode));
            _cursorPos++;
            _showCursor = true;
            _cursorBlinkTimer = 0.0f;
            if (_onChange) {
                _onChange(_text);
            }
        }
    }

    // Special keys
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        switch (keyPressed->key) {
            case events::Key::Backspace:
                if (_cursorPos > 0) {
                    _text.erase(_cursorPos - 1, 1);
                    _cursorPos--;
                    if (_onChange) {
                        _onChange(_text);
                    }
                }
                break;

            case events::Key::Left:
                if (_cursorPos > 0) {
                    _cursorPos--;
                }
                _showCursor = true;
                _cursorBlinkTimer = 0.0f;
                break;

            case events::Key::Right:
                if (_cursorPos < _text.length()) {
                    _cursorPos++;
                }
                _showCursor = true;
                _cursorBlinkTimer = 0.0f;
                break;

            case events::Key::Enter:
                if (_onSubmit) {
                    _onSubmit(_text);
                }
                break;

            default:
                break;
        }
    }
}

void TextInput::update(float deltaTime)
{
    if (_focused) {
        _cursorBlinkTimer += deltaTime;
        if (_cursorBlinkTimer >= CURSOR_BLINK_RATE) {
            _showCursor = !_showCursor;
            _cursorBlinkTimer = 0.0f;
        }
    }
}

void TextInput::render(graphics::IWindow& window)
{
    if (!_visible) return;

    rgba bgColor = _focused ? _focusedBgColor : _bgColor;
    rgba borderColor = _focused ? _focusedBorderColor : _borderColor;

    // Background
    window.drawRect(_pos.x, _pos.y, _size.x, _size.y, bgColor);

    // Border (2px)
    window.drawRect(_pos.x, _pos.y, _size.x, 2, borderColor);                          // Top
    window.drawRect(_pos.x, _pos.y + _size.y - 2, _size.x, 2, borderColor);            // Bottom
    window.drawRect(_pos.x, _pos.y, 2, _size.y, borderColor);                          // Left
    window.drawRect(_pos.x + _size.x - 2, _pos.y, 2, _size.y, borderColor);            // Right

    // Text positioning
    float textX = _pos.x + 10;
    float textY = _pos.y + (_size.y - 18) / 2.0f;
    unsigned int fontSize = 18;

    // Text or placeholder
    if (_text.empty() && !_focused) {
        window.drawText(_fontKey, _placeholder, textX, textY, fontSize, _placeholderColor);
    } else {
        std::string displayText = _isPassword ? std::string(_text.length(), '*') : _text;
        window.drawText(_fontKey, displayText, textX, textY, fontSize, _textColor);
    }

    // Cursor
    if (_focused && _showCursor) {
        float cursorX = textX + (static_cast<float>(_cursorPos) * DEFAULT_CHAR_WIDTH);
        float cursorY = _pos.y + 8;
        float cursorHeight = _size.y - 16;
        window.drawRect(cursorX, cursorY, 2, cursorHeight, _cursorColor);
    }
}

} // namespace ui
