#include "Panel.hpp"
#include "UIContext.hpp"

Panel::Panel()
{
    size = { 300.0f, 400.0f };
}

void Panel::setBackgroundColor(const glm::vec4& color) { backgroundColor = color; }
void Panel::setBorderColor(const glm::vec4& color) { borderColor = color; }
void Panel::setBorderThickness(float thickness) { borderThickness = thickness; }

void Panel::draw(UIContext& ctx)
{
    if (!visible) return;

    // Background
    ctx.drawRect(position, size, backgroundColor);

    // Simple border (4 thin rectangles)
    if (borderThickness > 0.0f)
    {
        // Top
        ctx.drawRect(position, { size.x, borderThickness }, borderColor);
        // Bottom
        ctx.drawRect({ position.x, position.y + size.y - borderThickness }, { size.x, borderThickness }, borderColor);
        // Left
        ctx.drawRect(position, { borderThickness, size.y }, borderColor);
        // Right
        ctx.drawRect({ position.x + size.x - borderThickness, position.y }, { borderThickness, size.y }, borderColor);
    }

    // Draw children
    Widget::draw(ctx);
}