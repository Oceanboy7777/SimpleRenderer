#include "HierarchyPanel.hpp"
#include "SceneDialog.hpp"
#include "ui/UIContext.hpp"
#include "ui/Font.hpp"
#include "engine/SceneSerializer.hpp"
#include "engine/ProjectManager.hpp"
#include "engine/GameObject.hpp"

#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace
{
    constexpr float kPadX = 10.0f;
    constexpr float kPadY = 8.0f;
    constexpr float kRowH = 30.0f;

    const char* categoryName(int i)
    {
        switch (i)
        {
        case 0: return "Gamespace";
        case 1: return "User Interface";
        case 2: return "Rendering";
        case 3: return "Sound Service";
        case 4: return "Physics Service";
        case 5: return "Script Service";
        default: return "Unknown";
        }
    }
}

HierarchyPanel::HierarchyPanel()
{
    setPosition({ 10.0f, 10.0f });
    setSize({ 280.0f, 520.0f });
    setBackgroundColor({ 0.12f, 0.13f, 0.16f, 0.95f });
    initCategories();

    searchBox.setPlaceholder("Search...");
    searchBox.setSize({ 256.0f, 24.0f });
    searchBox.setOnChanged([this](const std::string& text)
        {
            searchFilter = text;
        });
}

void HierarchyPanel::initCategories()
{
    for (int i = 0; i < static_cast<int>(Category::Count); ++i)
    {
        categories[i].name = categoryName(i);
        categories[i].expanded = (i == 0);
    }
}

void HierarchyPanel::setScene(Scene* s) { scene = s; }
void HierarchyPanel::setSceneDialog(SceneDialog* dialog) { sceneDialog = dialog; }
void HierarchyPanel::setCurrentScenePath(const std::filesystem::path& path) { currentScenePath = path; }
std::filesystem::path HierarchyPanel::getCurrentScenePath() const { return currentScenePath; }
void HierarchyPanel::setSelectedId(std::uint64_t id) { selectedId = id; }
void HierarchyPanel::setOnOpenUIDesigner(std::function<void(std::uint64_t)> cb) { onOpenUIDesigner = std::move(cb); }
std::uint64_t HierarchyPanel::getSelectedId() const { return selectedId; }

void HierarchyPanel::update(UIContext& ctx, float deltaTime)
{
    Panel::update(ctx, deltaTime);
    listScroll.update(ctx, deltaTime);
}

void HierarchyPanel::createStaticMesh(MeshType type, const std::string& name)
{
    if (!scene) return;

    static int counter = 1;
    const float offset = static_cast<float>(counter) * 0.55f;

    GameObject& obj = scene->createObject(
        name,
        glm::vec3(offset, 0.5f, offset * 0.25f),
        glm::vec3(0.7f),
        glm::vec3(0.45f + (counter % 5) * 0.08f, 0.65f, 0.85f),
        0.0f,
        type,
        MaterialType::Default);

    selectedId = obj.getId();
    ++counter;
    createMenuOpen = false;
    staticMeshMenuOpen = false;
    createMenuCategory = -1;
}

void HierarchyPanel::createNewUserInterface()
{
    if (!scene) return;
    GameObject& obj = scene->createUIObject("New User Interface", UIType::UIRoot, 0);
    selectedId = obj.getId();
    createMenuOpen = false;
    staticMeshMenuOpen = false;
    createMenuCategory = -1;
}


void HierarchyPanel::deleteSelected()
{
    if (!scene || selectedId == 0) return;
    GameObject* obj = scene->findObjectById(selectedId);

    if (scene->removeObject(selectedId))
    {
        if (renaming && renameId == selectedId) cancelRename();
        selectedId = 0;
        hoveredId = 0;
    }
}

void HierarchyPanel::saveScene()
{
    if (!scene || !sceneDialog) return;
    sceneDialog->openSave(currentScenePath);
}

void HierarchyPanel::loadScene()
{
    if (!scene || !sceneDialog) return;
    sceneDialog->openLoad();
}

void HierarchyPanel::beginRename()
{
    if (!scene || selectedId == 0) return;
    GameObject* obj = scene->findObjectById(selectedId);
    if (!obj) return;
    renaming = true;
    renameId = selectedId;
    renameBuffer = obj->getName();
}

void HierarchyPanel::commitRename()
{
    if (!scene || renameId == 0) return;
    if (!renameBuffer.empty())
    {
        if (!scene->tryRename(renameId, renameBuffer))
        {
            // name taken – keep editing / or just reject
            std::cout << "Name already in use: " << renameBuffer << "\n";
            return;
        }
    }
    renaming = false;
    renameId = 0;
    renameBuffer.clear();
}

void HierarchyPanel::cancelRename()
{
    renaming = false;
    renameId = 0;
    renameBuffer.clear();
}

void HierarchyPanel::onKey(int key, int action)
{
    searchBox.onKey(key, action);

    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    if (searchBox.isFocused())
        return;

    if (renaming)
    {
        if (key == GLFW_KEY_ENTER)
            commitRename();
        else if (key == GLFW_KEY_ESCAPE)
            cancelRename();
        else if (key == GLFW_KEY_BACKSPACE && !renameBuffer.empty())
            renameBuffer.pop_back();
        return;
    }

    if (key == GLFW_KEY_F2)
        beginRename();
    else if (key == GLFW_KEY_DELETE)
        deleteSelected();
}

void HierarchyPanel::onChar(unsigned int codepoint)
{
    searchBox.onChar(codepoint);

    if (searchBox.isFocused())
        return;

    if (!renaming)
        return;

    if (codepoint < 32 || codepoint > 126)
        return;

    if (renameBuffer.size() >= 48)
        return;

    renameBuffer.push_back(static_cast<char>(codepoint));
}

void HierarchyPanel::onScroll(float yOffset)
{
    listScroll.onScroll(yOffset);
}

void HierarchyPanel::draw(UIContext& ctx)
{
    if (!visible) return;

    itemRects.clear();
    actionHits.clear();
    Panel::draw(ctx);

    const glm::vec2 wp = getWorldPosition();

    Font::draw(ctx, wp + glm::vec2(kPadX, 10.0f),
        "Hierarchy", glm::vec4(0.85f, 0.9f, 1.0f, 1.0f), 1.7f);

    std::string sceneLabel = currentScenePath.empty()
        ? "Scene: (none)"
        : "Scene: " + currentScenePath.filename().string();
    Font::draw(ctx, wp + glm::vec2(kPadX, 32.0f),
        sceneLabel, glm::vec4(0.55f, 0.6f, 0.7f, 1.0f), 1.1f);

    // Search
    searchBox.setPosition(wp + glm::vec2(12.0f, 52.0f));
    searchBox.setSize({ size.x - 24.0f, 24.0f });
    searchBox.draw(ctx);

    // Save / Load / Del
    const float toolY = 84.0f;
    auto toolBtn = [&](float x, const char* label, ActionHit::Type type, glm::vec4 col)
        {
            glm::vec2 p = wp + glm::vec2(x, toolY);
            glm::vec2 s = { 80.0f, 26.0f };
            ctx.drawRect(p, s, col);
            float tw = Font::getTextWidth(label, 1.25f);
            Font::draw(ctx, p + glm::vec2((s.x - tw) * 0.5f, 6.0f), label, glm::vec4(1.0f), 1.25f);
            actionHits.push_back({ type, p, s, -1 });
        };
    toolBtn(12.0f, "Save", ActionHit::Type::SaveScene, { 0.25f, 0.35f, 0.55f, 1.0f });
    toolBtn(100.0f, "Load", ActionHit::Type::LoadScene, { 0.35f, 0.30f, 0.50f, 1.0f });
    toolBtn(188.0f, "Del", ActionHit::Type::Delete, { 0.50f, 0.20f, 0.20f, 1.0f });

    const float listTop = 120.0f;
    const float listH = size.y - listTop - 8.0f;
    listScroll.setPosition(wp + glm::vec2(8.0f, listTop));
    listScroll.setSize({ size.x - 16.0f, listH });

    float localY = 4.0f;
    const float ly = listScroll.getWorldPosition().y;
    const float lb = ly + listScroll.getSize().y;

    for (int c = 0; c < static_cast<int>(Category::Count); ++c)
    {
        const float worldY = listScroll.contentToWorldY(localY);
        glm::vec2 rowPos = { listScroll.getWorldPosition().x + 4.0f, worldY };
        glm::vec2 rowSize = { listScroll.getSize().x - 8.0f, kRowH };

        if (worldY + kRowH >= ly && worldY <= lb)
        {
            ctx.drawRect(rowPos, rowSize, glm::vec4(0.18f, 0.20f, 0.26f, 0.95f));
            const char* arrow = categories[c].expanded ? "v " : "> ";
            Font::draw(ctx, rowPos + glm::vec2(kPadX, kPadY),
                std::string(arrow) + categories[c].name,
                glm::vec4(0.85f, 0.88f, 1.0f, 1.0f), 1.3f);

            itemRects.push_back({ 0, rowPos, rowSize, true, c });

            // + button on category
            glm::vec2 plusSize = { 28.0f, 22.0f };
            glm::vec2 plusPos = {
                rowPos.x + rowSize.x - plusSize.x - 6.0f,
                rowPos.y + (kRowH - plusSize.y) * 0.5f
            };
            ctx.drawRect(plusPos, plusSize, glm::vec4(0.20f, 0.45f, 0.30f, 1.0f));
            Font::draw(ctx, plusPos + glm::vec2(8.0f, 4.0f), "+", glm::vec4(1.0f), 1.4f);
            actionHits.push_back({ ActionHit::Type::OpenCreateMenu, plusPos, plusSize, c });

            if (createMenuOpen && createMenuCategory == c)
                lastCreateButtonPos = plusPos;
        }
        localY += kRowH + 4.0f;

        if (!categories[c].expanded)
            continue;

        if (c == static_cast<int>(Category::Gamespace) && scene)
        {
            for (const auto& objPtr : scene->getObjects())
            {
                const GameObject& obj = *objPtr;
                if (obj.isUI()) continue; // UI objects live under User Interface category

                if (!searchFilter.empty())
                {
                    std::string lowerName = obj.getName();
                    std::string lowerFilter = searchFilter;
                    for (char& ch : lowerName)
                        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                    for (char& ch : lowerFilter)
                        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

                    if (lowerName.find(lowerFilter) == std::string::npos)
                        continue;
                }

                const float oy = listScroll.contentToWorldY(localY);
                glm::vec2 oPos = { listScroll.getWorldPosition().x + 16.0f, oy };
                glm::vec2 oSize = { listScroll.getSize().x - 24.0f, kRowH };

                if (oy + kRowH >= ly && oy <= lb)
                {
                    const bool isSelected = (obj.getId() == selectedId);
                    const bool isHovered = (obj.getId() == hoveredId);
                    if (isSelected)
                        ctx.drawRect(oPos, oSize, glm::vec4(0.22f, 0.38f, 0.72f, 0.95f));
                    else if (isHovered)
                        ctx.drawRect(oPos, oSize, glm::vec4(0.22f, 0.24f, 0.30f, 0.9f));
                    else
                        ctx.drawRect(oPos, oSize, glm::vec4(0.14f, 0.15f, 0.18f, 0.7f));

                    std::string label = obj.getName();
                    if (renaming && renameId == obj.getId())
                        label = renameBuffer + "_";

                    Font::draw(ctx, oPos + glm::vec2(kPadX, kPadY), label, glm::vec4(0.95f), 1.25f);
                    itemRects.push_back({ obj.getId(), oPos, oSize, false, -1, false });
                }
                localY += kRowH + 4.0f;
            }
        }
        else if (c == static_cast<int>(Category::UserInterface) && scene)
        {
            bool any = false;
            for (const auto& objPtr : scene->getObjects())
            {
                const GameObject& obj = *objPtr;
                if (!obj.isUIRoot()) continue;

                if (!searchFilter.empty())
                {
                    std::string lowerName = obj.getName();
                    std::string lowerFilter = searchFilter;
                    for (char& ch : lowerName)
                        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                    for (char& ch : lowerFilter)
                        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                    if (lowerName.find(lowerFilter) == std::string::npos)
                        continue;
                }

                any = true;
                const float oy = listScroll.contentToWorldY(localY);
                glm::vec2 oPos = { listScroll.getWorldPosition().x + 16.0f, oy };
                glm::vec2 oSize = { listScroll.getSize().x - 24.0f, kRowH };

                if (oy + kRowH >= ly && oy <= lb)
                {
                    const bool isSelected = (obj.getId() == selectedId);
                    const bool isHovered = (obj.getId() == hoveredId);
                    if (isSelected)
                        ctx.drawRect(oPos, oSize, glm::vec4(0.22f, 0.38f, 0.72f, 0.95f));
                    else if (isHovered)
                        ctx.drawRect(oPos, oSize, glm::vec4(0.22f, 0.24f, 0.30f, 0.9f));
                    else
                        ctx.drawRect(oPos, oSize, glm::vec4(0.14f, 0.15f, 0.18f, 0.7f));

                    std::string label = obj.getName();
                    if (renaming && renameId == obj.getId())
                        label = renameBuffer + "_";

                    Font::draw(ctx, oPos + glm::vec2(kPadX, kPadY), label, glm::vec4(0.95f), 1.25f);
                    itemRects.push_back({ obj.getId(), oPos, oSize, false, -1, true });

                    // small "Edit" button to open UI Designer
                    glm::vec2 editSize = { 40.0f, 20.0f };
                    glm::vec2 editPos = { oPos.x + oSize.x - editSize.x - 4.0f, oPos.y + 5.0f };
                    ctx.drawRect(editPos, editSize, glm::vec4(0.25f, 0.45f, 0.70f, 1.0f));
                    Font::draw(ctx, editPos + glm::vec2(6.0f, 3.0f), "Edit", glm::vec4(1.0f), 1.1f);
                    actionHits.push_back({ ActionHit::Type::OpenUIDesigner, editPos, editSize, -1, obj.getId() });
                }
                localY += kRowH + 4.0f;
            }
            if (!any)
            {
                const float ey = listScroll.contentToWorldY(localY);
                if (ey + kRowH >= ly && ey <= lb)
                {
                    Font::draw(ctx,
                        glm::vec2(listScroll.getWorldPosition().x + 20.0f, ey + kPadY),
                        "(empty)", glm::vec4(0.45f), 1.15f);
                }
                localY += kRowH;
            }
        }
        else
        {
            const float ey = listScroll.contentToWorldY(localY);
            if (ey + kRowH >= ly && ey <= lb)
            {
                Font::draw(ctx,
                    glm::vec2(listScroll.getWorldPosition().x + 20.0f, ey + kPadY),
                    "(empty)", glm::vec4(0.45f), 1.15f);
            }
            localY += kRowH;
        }
    }

    listContentHeight = localY + 8.0f;
    listScroll.setContentHeight(listContentHeight);
    listScroll.draw(ctx);

    // Create popup
    if (createMenuOpen)
    {
        glm::vec2 menuPos = lastCreateButtonPos + glm::vec2(0.0f, 28.0f);
        const bool gamespace = (createMenuCategory == 0);
        glm::vec2 menuSize = {
            170.0f,
            gamespace ? (staticMeshMenuOpen ? 220.0f : 44.0f) : 44.0f
        };

        lastCreateMenuPos = menuPos;
        lastCreateMenuSize = menuSize;

        ctx.drawRect(menuPos, menuSize, glm::vec4(0.16f, 0.17f, 0.22f, 0.98f));

        if (createMenuCategory == static_cast<int>(Category::UserInterface))
        {
            glm::vec2 btnPos = menuPos + glm::vec2(8.0f, 8.0f);
            glm::vec2 btnSize = { 154.0f, 28.0f };
            ctx.drawRect(btnPos, btnSize, glm::vec4(0.22f, 0.40f, 0.55f, 1.0f));
            Font::draw(ctx, btnPos + glm::vec2(10.0f, 7.0f), "New User Interface", glm::vec4(1.0f), 1.2f);
            actionHits.push_back({ ActionHit::Type::CreateNewUserInterface, btnPos, btnSize, -1, 0 });
            menuSize.y = 44.0f;
            lastCreateMenuSize = menuSize;
        }
        else if (!gamespace)
        {
            Font::draw(ctx, menuPos + glm::vec2(12.0f, 14.0f),
                "(nothing yet)", glm::vec4(0.6f), 1.2f);
        }
        else
        {
            glm::vec2 smPos = menuPos + glm::vec2(8.0f, 8.0f);
            glm::vec2 smSize = { 154.0f, 28.0f };
            ctx.drawRect(smPos, smSize, glm::vec4(0.22f, 0.28f, 0.36f, 1.0f));
            Font::draw(ctx, smPos + glm::vec2(10.0f, 7.0f),
                staticMeshMenuOpen ? "Static Mesh v" : "Static Mesh >",
                glm::vec4(1.0f), 1.25f);
            actionHits.push_back({ ActionHit::Type::ToggleStaticMeshMenu, smPos, smSize, -1 });

            if (staticMeshMenuOpen)
            {
                struct Opt { const char* label; ActionHit::Type type; };
                const Opt opts[] = {
                    { "Cube", ActionHit::Type::CreateStaticCube },
                    { "Plane", ActionHit::Type::CreateStaticPlane },
                    { "Pyramid", ActionHit::Type::CreateStaticPyramid },
                    { "Custom", ActionHit::Type::CreateStaticCustom },
                    { "Sphere", ActionHit::Type::CreateStaticSphere },
                    { "Cone", ActionHit::Type::CreateStaticCone },
                };
                float my = 42.0f;
                for (const auto& o : opts)
                {
                    glm::vec2 p = menuPos + glm::vec2(12.0f, my);
                    glm::vec2 s = { 146.0f, 26.0f };
                    ctx.drawRect(p, s, glm::vec4(0.20f, 0.22f, 0.28f, 1.0f));
                    float tw = Font::getTextWidth(o.label, 1.2f);
                    Font::draw(ctx, p + glm::vec2((s.x - tw) * 0.5f, 6.0f),
                        o.label, glm::vec4(0.95f), 1.2f);
                    actionHits.push_back({ o.type, p, s, -1 });
                    my += 28.0f;
                }
            }
        }
    }
    else
    {
        lastCreateMenuSize = { 0.0f, 0.0f };
    }
}

bool HierarchyPanel::onMouseMove(const glm::vec2& worldMousePos)
{
    hoveredId = 0;
    listScroll.onMouseMove(worldMousePos);
    if (!visible || !enabled) return Panel::onMouseMove(worldMousePos);

    for (const auto& item : itemRects)
    {
        if (item.isCategory) continue;
        bool inside =
            worldMousePos.x >= item.pos.x && worldMousePos.x <= item.pos.x + item.size.x &&
            worldMousePos.y >= item.pos.y && worldMousePos.y <= item.pos.y + item.size.y;
        if (inside) { hoveredId = item.id; break; }
    }
    return Panel::onMouseMove(worldMousePos);
}

bool HierarchyPanel::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled)
        return false;

    if (searchBox.onMouseButton(button, action, worldMousePos))
        return true;

    if (Panel::onMouseButton(button, action, worldMousePos))
        return true;

    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return false;

    for (const auto& hit : actionHits)
    {
        bool inside =
            worldMousePos.x >= hit.pos.x && worldMousePos.x <= hit.pos.x + hit.size.x &&
            worldMousePos.y >= hit.pos.y && worldMousePos.y <= hit.pos.y + hit.size.y;
        if (!inside) continue;

        switch (hit.type)
        {
        case ActionHit::Type::OpenCreateMenu:
            if (createMenuOpen && createMenuCategory == hit.categoryIndex)
            {
                createMenuOpen = false;
                staticMeshMenuOpen = false;
                createMenuCategory = -1;
            }
            else
            {
                createMenuOpen = true;
                staticMeshMenuOpen = false;
                createMenuCategory = hit.categoryIndex;
            }
            return true;
        case ActionHit::Type::ToggleStaticMeshMenu:
            staticMeshMenuOpen = !staticMeshMenuOpen;
            return true;
        case ActionHit::Type::CreateStaticCube:
            createStaticMesh(MeshType::Cube, "Cube"); return true;
        case ActionHit::Type::CreateStaticPlane:
            createStaticMesh(MeshType::Plane, "Plane"); return true;
        case ActionHit::Type::CreateStaticPyramid:
            createStaticMesh(MeshType::ImportedModel, "Pyramid"); return true;
        case ActionHit::Type::CreateStaticCustom:
            createStaticMesh(MeshType::Cube, "StaticMesh"); return true;
        case ActionHit::Type::CreateStaticSphere:
            std::cout << "Sphere mesh not implemented yet\n";
            createMenuOpen = false; staticMeshMenuOpen = false; return true;
        case ActionHit::Type::CreateStaticCone:
            std::cout << "Cone mesh not implemented yet\n";
            createMenuOpen = false; staticMeshMenuOpen = false; return true;
        case ActionHit::Type::Delete:
            deleteSelected(); return true;
        case ActionHit::Type::SaveScene:
            saveScene(); return true;
        case ActionHit::Type::LoadScene:
            loadScene(); return true;
        case ActionHit::Type::CreateNewUserInterface:
            createNewUserInterface(); return true;
        case ActionHit::Type::OpenUIDesigner:
            if (onOpenUIDesigner) onOpenUIDesigner(hit.targetId);
            return true;
        }
    }

    if (createMenuOpen && lastCreateMenuSize.x > 0.0f)
    {
        bool insideMenu =
            worldMousePos.x >= lastCreateMenuPos.x &&
            worldMousePos.x <= lastCreateMenuPos.x + lastCreateMenuSize.x &&
            worldMousePos.y >= lastCreateMenuPos.y &&
            worldMousePos.y <= lastCreateMenuPos.y + lastCreateMenuSize.y;
        if (insideMenu) return true;
    }

    for (const auto& item : itemRects)
    {
        bool inside =
            worldMousePos.x >= item.pos.x && worldMousePos.x <= item.pos.x + item.size.x &&
            worldMousePos.y >= item.pos.y && worldMousePos.y <= item.pos.y + item.size.y;
        if (!inside) continue;

        if (item.isCategory)
        {
            categories[item.categoryIndex].expanded = !categories[item.categoryIndex].expanded;
            return true;
        }
        selectedId = item.id;
        if (renaming && renameId != selectedId) cancelRename();
        return true;
    }

    createMenuOpen = false;
    staticMeshMenuOpen = false;
    createMenuCategory = -1;
    return false;
}