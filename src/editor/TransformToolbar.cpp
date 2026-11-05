#include "TransformToolbar.hpp"
#include "ui/UIContext.hpp"
#include "ui/Font.hpp"

#include <GLFW/glfw3.h>

TransformToolbar::TransformToolbar()
{
    setSize({ 168.0f, 36.0f });
    setBackgroundColor({ 0.0f, 0.0f, 0.0f, 0.0f });
    setBorderThickness(0.0f);
}

void TransformToolbar::setMode(GizmoMode m) { mode = m; }
GizmoMode TransformToolbar::getMode() const { return mode; }
void TransformToolbar::setOnModeChanged(std::function<void(GizmoMode)> cb) { onModeChanged = std::move(cb); }

void TransformToolbar::draw(UIContext& ctx)
{
    if (!visible)
        return;

    hits.clear();
    // No Panel::draw — buttons only, no large background frame

    const glm::vec2 wp = getWorldPosition();

    auto btn = [&](float x, const char* label, GizmoMode m)
    {
        glm::vec2 p = wp + glm::vec2(x, 6.0f);
        glm::vec2 s = { 50.0f, 24.0f };
        const bool active = (mode == m);
        ctx.drawRect(p, s, active
            ? glm::vec4(0.22f, 0.38f, 0.72f, 0.95f)
            : glm::vec4(0.16f, 0.17f, 0.21f, 0.90f));
        const float tw = Font::getTextWidth(label, 1.05f);
        Font::draw(ctx, p + glm::vec2((s.x - tw) * 0.5f, 5.0f),
            label, glm::vec4(1.0f), 1.05f);
        hits.push_back({ m, p, s });
    };

    btn(4.0f, "Pos", GizmoMode::Translate);
    btn(58.0f, "Rot", GizmoMode::Rotate);
    btn(112.0f, "Scale", GizmoMode::Scale);
}

bool TransformToolbar::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled)
        return false;

    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return false;

    for (const auto& h : hits)
    {
        const bool inside =
            worldMousePos.x >= h.pos.x && worldMousePos.x <= h.pos.x + h.size.x &&
            worldMousePos.y >= h.pos.y && worldMousePos.y <= h.pos.y + h.size.y;
        if (!inside)
            continue;
        mode = h.mode;
        if (onModeChanged)
            onModeChanged(mode);
        return true;
    }
    return false;
}
