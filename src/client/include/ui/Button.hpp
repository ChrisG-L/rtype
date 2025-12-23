/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Button
*/

#ifndef BUTTON_HPP_
#define BUTTON_HPP_

#include "IUIElement.hpp"
#include "../graphics/IWindow.hpp"
#include "../events/Event.hpp"
#include "../utils/Vecs.hpp"
#include <string>
#include <functional>

namespace ui {

class Button : public IUIElement {
public:
    enum class State {
        Normal,
        Hovered,
        Pressed,
        Disabled
    };

    using ClickCallback = std::function<void()>;

    Button(const Vec2f& pos, const Vec2f& size, const std::string& text,
           const std::string& fontKey);
    ~Button() override = default;

    void setOnClick(ClickCallback callback) { _onClick = std::move(callback); }
    void setText(const std::string& text) { _text = text; }
    const std::string& getText() const { return _text; }

    void setNormalColor(rgba color) { _normalColor = color; }
    void setHoveredColor(rgba color) { _hoveredColor = color; }
    void setPressedColor(rgba color) { _pressedColor = color; }
    void setTextColor(rgba color) { _textColor = color; }

    State getState() const { return _state; }

    // IUIElement interface
    Vec2f getPos() const override { return _pos; }
    void setPos(const Vec2f& pos) override { _pos = pos; }
    Vec2f getSize() const override { return _size; }
    void setSize(const Vec2f& size) override { _size = size; }
    bool isFocused() const override { return _focused; }
    void setFocused(bool focused) override { _focused = focused; }

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render(graphics::IWindow& window) override;

private:
    Vec2f _pos;
    Vec2f _size;
    std::string _text;
    std::string _fontKey;
    State _state = State::Normal;
    bool _focused = false;

    rgba _normalColor{60, 60, 80, 255};
    rgba _hoveredColor{80, 80, 110, 255};
    rgba _pressedColor{40, 40, 60, 255};
    rgba _disabledColor{40, 40, 40, 128};
    rgba _textColor{255, 255, 255, 255};
    rgba _borderColor{100, 100, 130, 255};

    ClickCallback _onClick;
};

} // namespace ui

#endif /* !BUTTON_HPP_ */
