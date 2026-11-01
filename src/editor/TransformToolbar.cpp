#include "TransformToolbar.hpp"
#include "ui/UIContext.hpp"
#include "ui/Font.hpp"

#include <GLFW/glfw3.h>

TransformToolbar::TransformToolbar()
{
    setSize({ 168.0f, 36.0f });
    setBackgroundColor({ 0.12f, 0.13f, 0.16f, 0.95f });
}

void TransformToolbar::setMode(GizmoMode m)
{
    mode = m;
}

GizmoMode TransformToolbar::getMode() const
{
    return mode;
}

void TransformToolbar::setOnModeChanged(std::function<void(GizmoMode)> cb)
{
    onModeChanged = std::move(cb);
}

void TransformToolbar::draw(UIContext& ctx)
{
    if (!visible)
        return;

    hits.clear();
    Panel::draw(ctx);

    const glm::vec2 wp = getWorldPosition();
    const char* labels[3] = { "Pos", "Rot", "Scale" };
    const GizmoMode modes[3] = {
        GizmoMode::Translate,
        GizmoMode::Rotate,
        GizmoMode::Scale
    };

    float x = 6.0f;
    for (int i = 0; i < 3; ++i)
    {
        glm::vec2 p = wp + glm::vec2(x, 6.0f);
        glm::vec2 s = { 50.0f, 24.0f };
        const bool active = (mode == modes[i]);

        ctx.drawRect(p, s, active
            ? glm::vec4(0.25f, 0.45f, 0.70f, 1.0f)
            : glm::vec4(0.20f, 0.22f, 0.28f, 1.0f));

        const float tw = Font::getTextWidth(labels[i], 1.2f);
        Font::draw(ctx, p + glm::vec2((s.x - tw) * 0.5f, 5.0f),
            labels[i], glm::vec4(1.0f), 1.2f);

        hits.push_back({ modes[i], p, s });
        x += 54.0f;
    }
}

bool TransformToolbar::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled)
        return false;

    if (Panel::onMouseButton(button, action, worldMousePos))
        return true;

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

    return containsPoint(worldMousePos);
}