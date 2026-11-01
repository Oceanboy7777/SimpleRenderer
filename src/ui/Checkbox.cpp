#include "Checkbox.hpp"
#include "UIContext.hpp"
#include "Font.hpp"
#include <GLFW/glfw3.h>

Checkbox::Checkbox()
{
    size = { 200.0f, 22.0f };
}

void Checkbox::setLabel(const std::string& text) { label = text; }
void Checkbox::setChecked(bool c) { checked = c; }
bool Checkbox::isChecked() const { return checked; }
void Checkbox::setOnChanged(std::function<void(bool)> cb) { onChanged = std::move(cb); }

void Checkbox::draw(UIContext& ctx)
{
    if (!visible) return;

    const glm::vec2 wp = getWorldPosition();
    const float box = 16.0f;

    ctx.drawRect(wp, { box, box }, glm::vec4(0.18f, 0.19f, 0.23f, 1.0f));
    ctx.drawRect(wp, { box, 1.0f }, glm::vec4(0.4f));
    ctx.drawRect(wp + glm::vec2(0, box - 1), { box, 1.0f }, glm::vec4(0.4f));
    ctx.drawRect(wp, { 1.0f, box }, glm::vec4(0.4f));
    ctx.drawRect(wp + glm::vec2(box - 1, 0), { 1.0f, box }, glm::vec4(0.4f));

    if (checked)
        ctx.drawRect(wp + glm::vec2(3, 3), { box - 6, box - 6 }, glm::vec4(0.3f, 0.7f, 0.4f, 1.0f));

    if (!label.empty())
        Font::draw(ctx, wp + glm::vec2(box + 8.0f, 2.0f), label, glm::vec4(0.92f), 1.3f);

    Widget::draw(ctx);
}

bool Checkbox::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled) return false;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && containsPoint(worldMousePos))
    {
        checked = !checked;
        if (onChanged) onChanged(checked);
        return true;
    }
    return Widget::onMouseButton(button, action, worldMousePos);
}