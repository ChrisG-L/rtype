/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** IUIElement
*/

#ifndef IUIELEMENT_HPP_
#define IUIELEMENT_HPP_

#include "../utils/Vecs.hpp"
#include "../events/Event.hpp"

namespace graphics {
    class IWindow;
}

namespace ui {

class IUIElement {
public:
    virtual ~IUIElement() = default;

    virtual Vec2f getPos() const = 0;
    virtual void setPos(const Vec2f& pos) = 0;
    virtual Vec2f getSize() const = 0;
    virtual void setSize(const Vec2f& size) = 0;

    virtual bool contains(float x, float y) const {
        Vec2f pos = getPos();
        Vec2f size = getSize();
        return x >= pos.x && x <= pos.x + size.x &&
               y >= pos.y && y <= pos.y + size.y;
    }

    virtual void handleEvent(const events::Event& event) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render(graphics::IWindow& window) = 0;

    virtual bool isFocused() const = 0;
    virtual void setFocused(bool focused) = 0;

    virtual bool isVisible() const { return _visible; }
    virtual void setVisible(bool visible) { _visible = visible; }

    virtual bool isEnabled() const { return _enabled; }
    virtual void setEnabled(bool enabled) { _enabled = enabled; }

protected:
    bool _visible = true;
    bool _enabled = true;
};

} // namespace ui

#endif /* !IUIELEMENT_HPP_ */
