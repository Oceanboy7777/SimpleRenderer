#include "ScrollFrame.hpp"
#include "UIContext.hpp"
#include <algorithm>

ScrollFrame::ScrollFrame()
{
    size = { 200.0f, 200.0f };
}

void ScrollFrame::setContentHeight(float height)
{
    contentHeight = height;
    scrollY = std::clamp(scrollY, 0.0f, maxScroll());
}

float ScrollFrame::getScrollY() const { return scrollY; }

void ScrollFrame::setScrollY(float y)
{
    scrollY = std::clamp(y, 0.0f, maxScroll());
}

float ScrollFrame::maxScroll() const
{
    return std::max(0.0f, contentHeight - size.y);
}

float ScrollFrame::contentToWorldY(float localY) const
{
    return getWorldPosition().y + localY - scrollY;
}

void ScrollFrame::draw(UIContext& ctx)
{
    if (!visible) return;

    const glm::vec2 wp = getWorldPosition();
    ctx.drawRect(wp, size, glm::vec4(0.10f, 0.11f, 0.13f, 0.5f));

    // Scrollbar
    if (contentHeight > size.y + 1.0f)
    {
        const float ratio = size.y / contentHeight;
        const float barH = std::max(24.0f, size.y * ratio);
        const float t = (maxScroll() > 0.0f) ? (scrollY / maxScroll()) : 0.0f;
        const float barY = wp.y + t * (size.y - barH);
        ctx.drawRect({ wp.x + size.x - 6.0f, barY }, { 5.0f, barH }, glm::vec4(0.35f, 0.38f, 0.45f, 0.9f));
    }

    Widget::draw(ctx);
}

bool ScrollFrame::onMouseMove(const glm::vec2& worldMousePos)
{
    hovered = containsPoint(worldMousePos);
    return Widget::onMouseMove(worldMousePos);
}

bool ScrollFrame::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    return Widget::onMouseButton(button, action, worldMousePos);
}

void ScrollFrame::onScroll(float yOffset)
{
    if (!hovered || !visible) return;
    // GLFW scroll: positive = up
    setScrollY(scrollY - yOffset * 28.0f);
}