/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** TextInput
*/

#ifndef TEXTINPUT_HPP_
#define TEXTINPUT_HPP_

#include "IUIElement.hpp"
#include "../graphics/IWindow.hpp"
#include "../events/Event.hpp"
#include "../utils/Vecs.hpp"
#include <string>
#include <functional>

namespace ui {

class TextInput : public IUIElement {
public:
    using OnChangeCallback = std::function<void(const std::string&)>;
    using OnSubmitCallback = std::function<void(const std::string&)>;

    TextInput(const Vec2f& pos, const Vec2f& size, const std::string& placeholder,
              const std::string& fontKey, bool isPassword = false);
    ~TextInput() override = default;

    const std::string& getText() const { return _text; }
    void setText(const std::string& text);
    void setPlaceholder(const std::string& placeholder) { _placeholder = placeholder; }
    void setMaxLength(size_t len) { _maxLength = len; }
    void setPassword(bool isPassword) { _isPassword = isPassword; }
    void clear();

    void setOnChange(OnChangeCallback callback) { _onChange = std::move(callback); }
    void setOnSubmit(OnSubmitCallback callback) { _onSubmit = std::move(callback); }

    void setBgColor(rgba color) { _bgColor = color; }
    void setFocusedBgColor(rgba color) { _focusedBgColor = color; }
    void setBorderColor(rgba color) { _borderColor = color; }
    void setFocusedBorderColor(rgba color) { _focusedBorderColor = color; }
    void setTextColor(rgba color) { _textColor = color; }
    void setPlaceholderColor(rgba color) { _placeholderColor = color; }

    // IUIElement interface
    Vec2f getPos() const override { return _pos; }
    void setPos(const Vec2f& pos) override { _pos = pos; }
    Vec2f getSize() const override { return _size; }
    void setSize(const Vec2f& size) override { _size = size; }
    bool isFocused() const override { return _focused; }
    void setFocused(bool focused) override;

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render(graphics::IWindow& window) override;

private:
    Vec2f _pos;
    Vec2f _size;
    std::string _text;
    std::string _placeholder;
    std::string _fontKey;
    bool _focused = false;
    bool _isPassword = false;
    size_t _cursorPos = 0;
    float _cursorBlinkTimer = 0.0f;
    bool _showCursor = true;
    size_t _maxLength = 255;

    rgba _bgColor{30, 30, 40, 255};
    rgba _focusedBgColor{40, 40, 55, 255};
    rgba _borderColor{80, 80, 100, 255};
    rgba _focusedBorderColor{100, 150, 255, 255};
    rgba _textColor{255, 255, 255, 255};
    rgba _placeholderColor{128, 128, 128, 255};
    rgba _cursorColor{255, 255, 255, 255};

    OnChangeCallback _onChange;
    OnSubmitCallback _onSubmit;

    static constexpr float CURSOR_BLINK_RATE = 0.5f;
    static constexpr float DEFAULT_CHAR_WIDTH = 10.0f;  // Approximate character width
};

} // namespace ui

#endif /* !TEXTINPUT_HPP_ */
