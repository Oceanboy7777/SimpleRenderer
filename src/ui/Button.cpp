#include "Button.hpp"
#include "UIContext.hpp"

Button::Button()
{
    size = { 120.0f, 32.0f };
}

void Button::setLabel(const std::string& text) { label = text; }
void Button::setOnClick(std::function<void()> callback) { onClick = std::move(callback); }

void Button::setNormalColor(const glm::vec4& color) { normalColor = color; }
void Button::setHoveredColor(const glm::vec4& color) { hoveredColor = color; }
void Button::setPressedColor(const glm::vec4& color) { pressedColor = color; }

void Button::draw(UIContext& ctx)
{
    if (!visible) return;

    glm::vec4 color = normalColor;
    if (pressed)      color = pressedColor;
    else if (hovered) color = hoveredColor;

    ctx.drawRect(position, size, color);

    // Text will come later – for now just the colored rectangle
    Widget::draw(ctx);
}

bool Button::onMouseButton(int button, int action, const glm::vec2& mousePos)
{
    if (!visible || !enabled) return false;

    const bool inside =
        mousePos.x >= position.x &&
        mousePos.x <= position.x + size.x &&
        mousePos.y >= position.y &&
        mousePos.y <= position.y + size.y;

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS && inside)
        {
            pressed = true;
            return true;
        }
        else if (action == GLFW_RELEASE)
        {
            if (pressed && inside && onClick)
            {
                onClick();
            }
            pressed = false;
            return inside;
        }
    }

    return Widget::onMouseButton(button, action, mousePos);
}