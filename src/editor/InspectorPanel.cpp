#include "InspectorPanel.hpp"
#include "ui/UIContext.hpp"
#include "ui/Font.hpp"
#include "engine/Transform.hpp"
#include "engine/ProjectManager.hpp"

#include <GLFW/glfw3.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <cctype>
#include <cstdlib>

namespace fs = std::filesystem;

InspectorPanel::InspectorPanel()
{
    setPosition({ 1000.0f, 10.0f });
    setSize({ 280.0f, 620.0f });
    setBackgroundColor({ 0.12f, 0.13f, 0.16f, 0.95f });
    setupFields();
}

void InspectorPanel::setupFields()
{
    for (int i = 0; i < static_cast<int>(Field::Count); ++i)
    {
        fields[i].setSize({ 90.0f, 24.0f });
        fields[i].setPlaceholder("0");

        Field f = static_cast<Field>(i);
        fields[i].setOnSubmit([this, f](const std::string& text)
            {
                applyField(f, text);
                syncFieldsFromObject();
            });
    }
}

void InspectorPanel::setScene(Scene* s)
{
    scene = s;
}

void InspectorPanel::setSelectedId(std::uint64_t id)
{
    if (selectedId != id)
    {
        selectedId = id;
        lastSyncedId = 0; // force resync
    }
}

float InspectorPanel::parseFloat(const std::string& text, float fallback)
{
    if (text.empty()) return fallback;
    try { return std::stof(text); }
    catch (...) { return fallback; }
}

void InspectorPanel::syncFieldsFromObject()
{
    if (!scene || selectedId == 0) return;
    GameObject* obj = scene->findObjectById(selectedId);
    if (!obj) return;

    auto set = [&](Field f, float v)
        {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(2) << v;
            fields[static_cast<int>(f)].setText(ss.str());
        };

    glm::vec3 p = obj->getPosition();
    glm::vec3 s = obj->getScale();
    glm::vec3 r = obj->getTransform().getRotation();
    glm::vec3 c = obj->getColor();

    set(Field::PosX, p.x); set(Field::PosY, p.y); set(Field::PosZ, p.z);
    set(Field::ScaleX, s.x); set(Field::ScaleY, s.y); set(Field::ScaleZ, s.z);
    set(Field::RotX, r.x); set(Field::RotY, r.y); set(Field::RotZ, r.z);
    set(Field::ColR, c.r); set(Field::ColG, c.g); set(Field::ColB, c.b);

    lastSyncedId = selectedId;
}

void InspectorPanel::applyField(Field field, const std::string& text)
{
    if (!scene || selectedId == 0) return;
    GameObject* obj = scene->findObjectById(selectedId);
    if (!obj) return;

    Transform& t = obj->getTransform();
    float v = parseFloat(text, 0.0f);

    switch (field)
    {
    case Field::PosX: case Field::PosY: case Field::PosZ:
    {
        glm::vec3 p = obj->getPosition();
        if (field == Field::PosX) p.x = v;
        if (field == Field::PosY) p.y = v;
        if (field == Field::PosZ) p.z = v;
        t.setPosition(p);
        break;
    }
    case Field::ScaleX: case Field::ScaleY: case Field::ScaleZ:
    {
        glm::vec3 s = obj->getScale();
        if (field == Field::ScaleX) s.x = std::max(0.05f, v);
        if (field == Field::ScaleY) s.y = std::max(0.05f, v);
        if (field == Field::ScaleZ) s.z = std::max(0.05f, v);
        t.setScale(s);
        break;
    }
    case Field::RotX: case Field::RotY: case Field::RotZ:
    {
        glm::vec3 r = t.getRotation();
        if (field == Field::RotX) r.x = v;
        if (field == Field::RotY) r.y = v;
        if (field == Field::RotZ) r.z = v;
        t.setRotation(r);
        break;
    }
    case Field::ColR: case Field::ColG: case Field::ColB:
    {
        glm::vec3 c = obj->getColor();
        if (field == Field::ColR) c.r = std::clamp(v, 0.0f, 1.0f);
        if (field == Field::ColG) c.g = std::clamp(v, 0.0f, 1.0f);
        if (field == Field::ColB) c.b = std::clamp(v, 0.0f, 1.0f);
        obj->setColor(c);
        break;
    }
    default: break;
    }
}

void InspectorPanel::layoutAndDrawFields(UIContext& ctx, float& y)
{
    const glm::vec2 world = getWorldPosition();
    const float labelX = 14.0f;
    const float boxX = 160.0f;

    auto section = [&](const char* title)
        {
            Font::draw(ctx, world + glm::vec2(labelX, y),
                title, glm::vec4(0.55f, 0.75f, 1.0f, 1.0f), 1.35f);
            y += 26.0f;
        };

    auto row = [&](const char* label, Field f)
        {
            Font::draw(ctx, world + glm::vec2(labelX, y + 4.0f),
                label, glm::vec4(0.75f), 1.2f);

            TextBox& box = fields[static_cast<int>(f)];
            box.setPosition(world + glm::vec2(boxX, y));
            box.setSize({ 100.0f, 24.0f });
            box.draw(ctx);
            y += 30.0f;
        };

    section("Position");
    row("X", Field::PosX);
    row("Y", Field::PosY);
    row("Z", Field::PosZ);

    y += 6.0f;
    section("Scale");
    row("X", Field::ScaleX);
    row("Y", Field::ScaleY);
    row("Z", Field::ScaleZ);

    y += 6.0f;
    section("Rotation");
    row("X", Field::RotX);
    row("Y", Field::RotY);
    row("Z", Field::RotZ);

    y += 6.0f;
    section("Color");
    row("R", Field::ColR);
    row("G", Field::ColG);
    row("B", Field::ColB);
}

void InspectorPanel::drawMeshSection(UIContext& ctx, float& y, GameObject* obj)
{
    const glm::vec2 world = getWorldPosition();
    meshHits.clear();

    Font::draw(ctx, world + glm::vec2(14.0f, y),
        "Mesh Source", glm::vec4(0.55f, 0.75f, 1.0f, 1.0f), 1.35f);
    y += 26.0f;

    std::string current = obj->getMeshPath().empty()
        ? "(built-in)"
        : fs::path(obj->getMeshPath()).filename().string();
    Font::draw(ctx, world + glm::vec2(14.0f, y),
        "Current: " + current, glm::vec4(0.85f), 1.1f);
    y += 24.0f;

    {
        glm::vec2 p = world + glm::vec2(14.0f, y);
        glm::vec2 s = { 140.0f, 24.0f };
        ctx.drawRect(p, s, glm::vec4(0.35f, 0.25f, 0.25f, 1.0f));
        Font::draw(ctx, p + glm::vec2(12.0f, 5.0f), "Use Built-in", glm::vec4(1.0f), 1.1f);
        meshHits.push_back({ "", p, s });
        y += 30.0f;
    }

    if (!ProjectManager::current()) return;

    fs::path models = ProjectManager::modelsDir();
    if (!fs::exists(models)) return;

    int shown = 0;
    for (auto& entry : fs::directory_iterator(models))
    {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        for (char& c : ext)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ext != ".obj") continue;

        std::string full = entry.path().string();
        glm::vec2 p = world + glm::vec2(14.0f, y);
        glm::vec2 s = { size.x - 28.0f, 24.0f };
        bool active = (obj->getMeshPath() == full);
        ctx.drawRect(p, s, active
            ? glm::vec4(0.22f, 0.40f, 0.30f, 1.0f)
            : glm::vec4(0.18f, 0.19f, 0.23f, 1.0f));
        Font::draw(ctx, p + glm::vec2(10.0f, 5.0f),
            entry.path().filename().string(), glm::vec4(0.95f), 1.1f);
        meshHits.push_back({ full, p, s });
        y += 28.0f;
        if (++shown >= 8) break;
    }
}

void InspectorPanel::update(UIContext& ctx, float deltaTime)
{
    Panel::update(ctx, deltaTime);
    if (selectedId != 0 && selectedId != lastSyncedId)
        syncFieldsFromObject();
}

void InspectorPanel::draw(UIContext& ctx)
{
    if (!visible) return;
    meshHits.clear();
    Panel::draw(ctx);

    const glm::vec2 world = getWorldPosition();
    Font::draw(ctx, world + glm::vec2(14.0f, 12.0f),
        "Inspector", glm::vec4(0.85f, 0.9f, 1.0f, 1.0f), 1.8f);

    if (!scene || selectedId == 0)
    {
        Font::draw(ctx, world + glm::vec2(14.0f, 50.0f),
            "No selection", glm::vec4(0.6f), 1.3f);
        return;
    }

    GameObject* obj = scene->findObjectById(selectedId);
    if (!obj) return;

    if (lastSyncedId != selectedId)
        syncFieldsFromObject();

    float y = 48.0f;
    Font::draw(ctx, world + glm::vec2(14.0f, y),
        "Name: " + obj->getName(), glm::vec4(0.9f), 1.25f);
    y += 32.0f;

    layoutAndDrawFields(ctx, y);

    y += 8.0f;
    drawMeshSection(ctx, y, obj);
}

bool InspectorPanel::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled) return false;

    // Focus text boxes first (top fields)
    for (int i = 0; i < static_cast<int>(Field::Count); ++i)
    {
        if (fields[i].onMouseButton(button, action, worldMousePos))
        {
            // Unfocus others
            for (int j = 0; j < static_cast<int>(Field::Count); ++j)
                if (j != i) fields[j].setFocused(false);
            return true;
        }
    }

    if (Panel::onMouseButton(button, action, worldMousePos))
        return true;

    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return false;

    for (const auto& hit : meshHits)
    {
        bool inside =
            worldMousePos.x >= hit.pos.x && worldMousePos.x <= hit.pos.x + hit.size.x &&
            worldMousePos.y >= hit.pos.y && worldMousePos.y <= hit.pos.y + hit.size.y;
        if (!inside) continue;

        GameObject* obj = scene ? scene->findObjectById(selectedId) : nullptr;
        if (!obj) return true;
        if (hit.path.empty()) obj->clearMeshPath();
        else obj->setMeshPath(hit.path);
        return true;
    }

    // Click empty inspector → unfocus all
    for (int i = 0; i < static_cast<int>(Field::Count); ++i)
        fields[i].setFocused(false);

    return false;
}

void InspectorPanel::onKey(int key, int action)
{
    for (int i = 0; i < static_cast<int>(Field::Count); ++i)
        fields[i].onKey(key, action);
}

void InspectorPanel::onChar(unsigned int codepoint)
{
    for (int i = 0; i < static_cast<int>(Field::Count); ++i)
        fields[i].onChar(codepoint);
}