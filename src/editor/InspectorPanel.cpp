#include "InspectorPanel.hpp"
#include "ui/UIContext.hpp"
#include "ui/Font.hpp"
#include "engine/GameObject.hpp"
#include "engine/Transform.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>

InspectorPanel::InspectorPanel()
{
    setPosition({ 1000.0f, 10.0f });
    setSize({ 270.0f, 520.0f });
    setBackgroundColor({ 0.12f, 0.13f, 0.16f, 0.95f });
}

void InspectorPanel::setScene(Scene* s)
{
    scene = s;
}

void InspectorPanel::setSelectedId(std::uint64_t id)
{
    selectedId = id;
}

void InspectorPanel::update(UIContext& ctx, float deltaTime)
{
    Panel::update(ctx, deltaTime);
}

void InspectorPanel::drawFloatRow(UIContext& ctx, float& y,
    const std::string& label, float value,
    FieldType field, float step)
{
    const glm::vec2 world = getWorldPosition();

    // Label
    Font::draw(ctx, world + glm::vec2(14.0f, y),
        label, glm::vec4(0.70f, 0.75f, 0.85f, 1.0f), 1.3f);

    // Value
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << value;
    Font::draw(ctx, world + glm::vec2(100.0f, y),
        ss.str(), glm::vec4(1.0f), 1.3f);

    // − button
    glm::vec2 minusPos = world + glm::vec2(180.0f, y - 2.0f);
    glm::vec2 btnSize = { 28.0f, 22.0f };
    ctx.drawRect(minusPos, btnSize, glm::vec4(0.35f, 0.20f, 0.20f, 1.0f));
    Font::draw(ctx, minusPos + glm::vec2(8.0f, 4.0f), "-", glm::vec4(1.0f), 1.5f);
    buttonHits.push_back({ field, -step, minusPos, btnSize });

    // + button
    glm::vec2 plusPos = world + glm::vec2(214.0f, y - 2.0f);
    ctx.drawRect(plusPos, btnSize, glm::vec4(0.20f, 0.35f, 0.25f, 1.0f));
    Font::draw(ctx, plusPos + glm::vec2(8.0f, 4.0f), "+", glm::vec4(1.0f), 1.5f);
    buttonHits.push_back({ field, +step, plusPos, btnSize });

    y += 28.0f;
}

void InspectorPanel::draw(UIContext& ctx)
{
    if (!visible) return;

    buttonHits.clear();
    Panel::draw(ctx);

    const glm::vec2 world = getWorldPosition();

    Font::draw(ctx, world + glm::vec2(14.0f, 14.0f),
        "Inspector", glm::vec4(0.85f, 0.90f, 1.0f, 1.0f), 1.8f);

    if (!scene || selectedId == 0)
    {
        Font::draw(ctx, world + glm::vec2(14.0f, 50.0f),
            "No selection", glm::vec4(0.6f), 1.3f);
        return;
    }

    GameObject* obj = scene->findObjectById(selectedId);
    if (!obj) return;

    float y = 50.0f;

    // Name (read-only for now)
    Font::draw(ctx, world + glm::vec2(14.0f, y),
        "Name:", glm::vec4(0.70f, 0.75f, 0.85f, 1.0f), 1.3f);
    Font::draw(ctx, world + glm::vec2(100.0f, y),
        obj->getName(), glm::vec4(1.0f), 1.3f);
    y += 32.0f;

    // Position
    Font::draw(ctx, world + glm::vec2(14.0f, y),
        "Position", glm::vec4(0.55f, 0.75f, 1.0f, 1.0f), 1.4f);
    y += 26.0f;

    glm::vec3 pos = obj->getPosition();
    drawFloatRow(ctx, y, "X", pos.x, FieldType::PositionX, 0.25f);
    drawFloatRow(ctx, y, "Y", pos.y, FieldType::PositionY, 0.25f);
    drawFloatRow(ctx, y, "Z", pos.z, FieldType::PositionZ, 0.25f);

    y += 8.0f;

    // Scale
    Font::draw(ctx, world + glm::vec2(14.0f, y),
        "Scale", glm::vec4(0.55f, 0.75f, 1.0f, 1.0f), 1.4f);
    y += 26.0f;

    glm::vec3 scale = obj->getScale();
    drawFloatRow(ctx, y, "X", scale.x, FieldType::ScaleX, 0.1f);
    drawFloatRow(ctx, y, "Y", scale.y, FieldType::ScaleY, 0.1f);
    drawFloatRow(ctx, y, "Z", scale.z, FieldType::ScaleZ, 0.1f);

    y += 8.0f;

    // Color
    Font::draw(ctx, world + glm::vec2(14.0f, y),
        "Color", glm::vec4(0.55f, 0.75f, 1.0f, 1.0f), 1.4f);
    y += 26.0f;

    glm::vec3 color = obj->getColor();
    drawFloatRow(ctx, y, "R", color.r, FieldType::ColorR, 0.05f);
    drawFloatRow(ctx, y, "G", color.g, FieldType::ColorG, 0.05f);
    drawFloatRow(ctx, y, "B", color.b, FieldType::ColorB, 0.05f);
}

void InspectorPanel::applyChange(FieldType field, float delta)
{
    if (!scene || selectedId == 0) return;

    GameObject* obj = scene->findObjectById(selectedId);
    if (!obj) return;

    Transform& t = obj->getTransform();

    switch (field)
    {
    case FieldType::PositionX:
    case FieldType::PositionY:
    case FieldType::PositionZ:
    {
        glm::vec3 p = obj->getPosition();
        if (field == FieldType::PositionX) p.x += delta;
        if (field == FieldType::PositionY) p.y += delta;
        if (field == FieldType::PositionZ) p.z += delta;
        t.setPosition(p);
        break;
    }
    case FieldType::ScaleX:
    case FieldType::ScaleY:
    case FieldType::ScaleZ:
    {
        glm::vec3 s = obj->getScale();
        if (field == FieldType::ScaleX) s.x = std::max(0.05f, s.x + delta);
        if (field == FieldType::ScaleY) s.y = std::max(0.05f, s.y + delta);
        if (field == FieldType::ScaleZ) s.z = std::max(0.05f, s.z + delta);
        t.setScale(s);
        break;
    }
    case FieldType::ColorR:
    case FieldType::ColorG:
    case FieldType::ColorB:
    {
        // We need a setter for color – see note below
        glm::vec3 c = obj->getColor();
        if (field == FieldType::ColorR) c.r = std::clamp(c.r + delta, 0.0f, 1.0f);
        if (field == FieldType::ColorG) c.g = std::clamp(c.g + delta, 0.0f, 1.0f);
        if (field == FieldType::ColorB) c.b = std::clamp(c.b + delta, 0.0f, 1.0f);
        obj->setColor(c);
        break;
    }
    }
}

bool InspectorPanel::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled) return false;

    if (Panel::onMouseButton(button, action, worldMousePos))
        return true;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        for (const auto& hit : buttonHits)
        {
            bool inside =
                worldMousePos.x >= hit.pos.x &&
                worldMousePos.x <= hit.pos.x + hit.size.x &&
                worldMousePos.y >= hit.pos.y &&
                worldMousePos.y <= hit.pos.y + hit.size.y;

            if (inside)
            {
                applyChange(hit.field, hit.delta);
                return true;
            }
        }
    }

    return false;
}