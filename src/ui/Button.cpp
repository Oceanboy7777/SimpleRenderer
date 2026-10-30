#include "Button.hpp"
#include "UIContext.hpp"
#include "Font.hpp"

Button::Button()
{
    size = { 140.0f, 32.0f };
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

    const glm::vec2 worldPos = getWorldPosition();
    ctx.drawRect(worldPos, size, color);

    // Draw label centered
    if (!label.empty())
    {
        float textW = Font::getTextWidth(label, 1.5f);
        float textH = Font::getLineHeight(1.5f);
        glm::vec2 textPos = worldPos + glm::vec2(
            (size.x - textW) * 0.5f,
            (size.y - textH) * 0.5f
        );
        Font::draw(ctx, textPos, label, glm::vec4(1.0f), 1.5f);
    }

    Widget::draw(ctx);
}

bool Button::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled) return false;

    const bool inside = containsPoint(worldMousePos);

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
                onClick();
            pressed = false;
            return inside;
        }
    }

    return Widget::onMouseButton(button, action, worldMousePos);
}