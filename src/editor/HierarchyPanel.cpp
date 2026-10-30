#include "HierarchyPanel.hpp"
#include "ui/UIContext.hpp"
#include "ui/Font.hpp"
#include "engine/SceneSerializer.hpp"

#include <iostream>

HierarchyPanel::HierarchyPanel()
{
    setPosition({ 10.0f, 10.0f });
    setSize({ 260.0f, 560.0f });
    setBackgroundColor({ 0.12f, 0.13f, 0.16f, 0.95f });
}

void HierarchyPanel::setScene(Scene* s) { scene = s; }
void HierarchyPanel::setSelectedId(std::uint64_t id) { selectedId = id; }
std::uint64_t HierarchyPanel::getSelectedId() const { return selectedId; }

void HierarchyPanel::update(UIContext& ctx, float deltaTime)
{
    Panel::update(ctx, deltaTime);
}

void HierarchyPanel::createCube()
{
    if (!scene) return;

    static int counter = 1;
    float offset = static_cast<float>(counter) * 0.6f;

    GameObject& obj = scene->createObject(
        "Cube " + std::to_string(counter),
        glm::vec3(offset, 0.5f, offset * 0.3f),
        glm::vec3(0.7f),
        glm::vec3(0.4f + (counter % 5) * 0.1f, 0.6f, 0.9f - (counter % 4) * 0.1f),
        0.0f,
        MeshType::Cube,
        MaterialType::Default
    );

    selectedId = obj.getId();
    ++counter;
    std::cout << "Created object ID: " << selectedId << "\n";
}

void HierarchyPanel::deleteSelected()
{
    if (!scene || selectedId == 0) return;

    GameObject* obj = scene->findObjectById(selectedId);
    if (obj && obj->getName() == "Ground")
    {
        std::cout << "Cannot delete Ground\n";
        return;
    }

    if (scene->removeObject(selectedId))
    {
        std::cout << "Deleted object ID: " << selectedId << "\n";
        selectedId = 0;
        hoveredId = 0;
    }
}

void HierarchyPanel::draw(UIContext& ctx)
{
    if (!visible) return;

    itemRects.clear();
    actionButtons.clear();

    Panel::draw(ctx);
    if (!scene) return;

    const glm::vec2 worldPos = getWorldPosition();

    Font::draw(ctx, worldPos + glm::vec2(14.0f, 14.0f),
        "Hierarchy", glm::vec4(0.85f, 0.9f, 1.0f, 1.0f), 1.8f);

    const auto& objects = scene->getObjects();
    float y = 48.0f;

    for (const auto& obj : objects)
    {
        const bool isSelected = (obj->getId() == selectedId);
        const bool isHovered = (obj->getId() == hoveredId);

        glm::vec2 itemPos = worldPos + glm::vec2(10.0f, y);
        glm::vec2 itemSize = { size.x - 20.0f, 28.0f };
        itemRects.push_back({ obj->getId(), itemPos, itemSize });

        if (isSelected)
            ctx.drawRect(itemPos, itemSize, glm::vec4(0.22f, 0.38f, 0.72f, 0.95f));
        else if (isHovered)
            ctx.drawRect(itemPos, itemSize, glm::vec4(0.22f, 0.24f, 0.30f, 0.9f));
        else
            ctx.drawRect(itemPos, itemSize, glm::vec4(0.16f, 0.17f, 0.21f, 0.6f));

        Font::draw(ctx, itemPos + glm::vec2(10.0f, 7.0f),
            obj->getName(), glm::vec4(0.95f, 0.95f, 0.97f, 1.0f), 1.4f);

        y += 32.0f;
        if (y > size.y - 90.0f) break;
    }

    // Row 1: + Cube | Delete
    float btnY = size.y - 78.0f;
    glm::vec2 btnSize = { 110.0f, 28.0f };

    glm::vec2 createPos = worldPos + glm::vec2(15.0f, btnY);
    ctx.drawRect(createPos, btnSize, glm::vec4(0.20f, 0.45f, 0.30f, 1.0f));
    Font::draw(ctx, createPos + glm::vec2(22.0f, 6.0f), "+ Cube", glm::vec4(1.0f), 1.4f);
    actionButtons.push_back({ ActionButton::Type::CreateCube, createPos, btnSize });

    glm::vec2 deletePos = worldPos + glm::vec2(135.0f, btnY);
    ctx.drawRect(deletePos, btnSize, glm::vec4(0.50f, 0.20f, 0.20f, 1.0f));
    Font::draw(ctx, deletePos + glm::vec2(28.0f, 6.0f), "Delete", glm::vec4(1.0f), 1.4f);
    actionButtons.push_back({ ActionButton::Type::Delete, deletePos, btnSize });

    // Row 2: Save | Load
    btnY = size.y - 42.0f;

    glm::vec2 savePos = worldPos + glm::vec2(15.0f, btnY);
    ctx.drawRect(savePos, btnSize, glm::vec4(0.25f, 0.35f, 0.55f, 1.0f));
    Font::draw(ctx, savePos + glm::vec2(32.0f, 6.0f), "Save", glm::vec4(1.0f), 1.4f);
    actionButtons.push_back({ ActionButton::Type::SaveScene, savePos, btnSize });

    glm::vec2 loadPos = worldPos + glm::vec2(135.0f, btnY);
    ctx.drawRect(loadPos, btnSize, glm::vec4(0.35f, 0.30f, 0.50f, 1.0f));
    Font::draw(ctx, loadPos + glm::vec2(32.0f, 6.0f), "Load", glm::vec4(1.0f), 1.4f);
    actionButtons.push_back({ ActionButton::Type::LoadScene, loadPos, btnSize });
}

bool HierarchyPanel::onMouseMove(const glm::vec2& worldMousePos)
{
    hoveredId = 0;

    if (!visible || !enabled)
        return Panel::onMouseMove(worldMousePos);

    for (const auto& item : itemRects)
    {
        bool inside =
            worldMousePos.x >= item.pos.x &&
            worldMousePos.x <= item.pos.x + item.size.x &&
            worldMousePos.y >= item.pos.y &&
            worldMousePos.y <= item.pos.y + item.size.y;

        if (inside)
        {
            hoveredId = item.id;
            break;
        }
    }

    return Panel::onMouseMove(worldMousePos);
}

bool HierarchyPanel::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled) return false;
    if (Panel::onMouseButton(button, action, worldMousePos)) return true;
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return false;

    for (const auto& btn : actionButtons)
    {
        bool inside =
            worldMousePos.x >= btn.pos.x &&
            worldMousePos.x <= btn.pos.x + btn.size.x &&
            worldMousePos.y >= btn.pos.y &&
            worldMousePos.y <= btn.pos.y + btn.size.y;

        if (inside)
        {
            if (btn.type == ActionButton::Type::CreateCube) createCube();
            else if (btn.type == ActionButton::Type::Delete) deleteSelected();
            else if (btn.type == ActionButton::Type::SaveScene) saveScene();
            else if (btn.type == ActionButton::Type::LoadScene) loadScene();
            return true;
        }
    }

    for (const auto& item : itemRects)
    {
        bool inside =
            worldMousePos.x >= item.pos.x &&
            worldMousePos.x <= item.pos.x + item.size.x &&
            worldMousePos.y >= item.pos.y &&
            worldMousePos.y <= item.pos.y + item.size.y;

        if (inside)
        {
            selectedId = item.id;
            std::cout << "Selected object ID: " << selectedId << "\n";
            return true;
        }
    }

    return false;
}

void HierarchyPanel::saveScene()
{
    if (!scene) return;
    SceneSerializer::save(*scene);   // empty path → defaultScenePath()
}

void HierarchyPanel::loadScene()
{
    if (!scene) return;
    SceneSerializer::load(*scene);
    selectedId = 0;
    hoveredId = 0;
}