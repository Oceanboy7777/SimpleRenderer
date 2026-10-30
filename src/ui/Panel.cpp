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

    const glm::vec2 worldPos = getWorldPosition();

    // Background
    ctx.drawRect(worldPos, size, backgroundColor);

    // Border
    if (borderThickness > 0.0f)
    {
        ctx.drawRect(worldPos, { size.x, borderThickness }, borderColor);
        ctx.drawRect({ worldPos.x, worldPos.y + size.y - borderThickness }, { size.x, borderThickness }, borderColor);
        ctx.drawRect(worldPos, { borderThickness, size.y }, borderColor);
        ctx.drawRect({ worldPos.x + size.x - borderThickness, worldPos.y }, { borderThickness, size.y }, borderColor);
    }

    Widget::draw(ctx);
}