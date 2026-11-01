#include "SceneDialog.hpp"
#include "ui/UIContext.hpp"
#include "ui/Font.hpp"
#include "engine/ProjectManager.hpp"
#include "engine/SceneSerializer.hpp"

#include <GLFW/glfw3.h>
#include <iostream>

SceneDialog::SceneDialog()
{
    setPosition({ 340.0f, 180.0f });
    setSize({ 600.0f, 360.0f });
    setBackgroundColor({ 0.14f, 0.15f, 0.19f, 0.98f });
    setVisible(false);
}

void SceneDialog::setScene(Scene* s)
{
    scene = s;
}

void SceneDialog::setCurrentScenePath(const fs::path& path)
{
    currentScenePath = path;
}

fs::path SceneDialog::getCurrentScenePath() const
{
    return currentScenePath;
}

void SceneDialog::openSave(const fs::path& currentPath)
{
    mode = Mode::Save;
    if (!currentPath.empty())
        currentScenePath = currentPath;
    setVisible(true);
}

void SceneDialog::openLoad()
{
    mode = Mode::Load;
    sceneFiles = ProjectManager::listSceneFiles();
    setVisible(true);
}

void SceneDialog::close()
{
    mode = Mode::None;
    setVisible(false);
    hits.clear();
}

bool SceneDialog::isOpen() const
{
    return mode != Mode::None;
}

SceneDialog::Mode SceneDialog::getMode() const
{
    return mode;
}

void SceneDialog::doOverwrite()
{
    if (!scene)
        return;

    fs::path path = currentScenePath;
    if (path.empty() && ProjectManager::current())
        path = ProjectManager::scenesDir() / "main.scene";

    if (path.empty())
    {
        std::cerr << "[SceneDialog] No scene path to overwrite\n";
        return;
    }

    if (SceneSerializer::save(*scene, path.string()))
        currentScenePath = path;

    close();
}

void SceneDialog::doSaveAsNew()
{
    if (!scene || !ProjectManager::current())
        return;

    const std::string name = ProjectManager::nextSceneName("Scene");
    const fs::path path = ProjectManager::scenesDir() / name;

    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);

    if (SceneSerializer::save(*scene, path.string()))
    {
        currentScenePath = path;
        std::cout << "[SceneDialog] Saved new scene: " << path << "\n";
    }

    close();
}

void SceneDialog::doLoad(int index)
{
    if (!scene)
        return;

    if (index < 0 || index >= static_cast<int>(sceneFiles.size()))
        return;

    const fs::path& path = sceneFiles[index];
    if (SceneSerializer::load(*scene, path.string()))
    {
        currentScenePath = path;
        std::cout << "[SceneDialog] Loaded: " << path << "\n";
    }

    close();
}

void SceneDialog::update(UIContext& ctx, float deltaTime)
{
    if (!visible)
        return;

    Panel::update(ctx, deltaTime);
}

void SceneDialog::draw(UIContext& ctx)
{
    if (!visible || mode == Mode::None)
        return;

    hits.clear();
    Panel::draw(ctx);

    const glm::vec2 wp = getWorldPosition();

    if (mode == Mode::Save)
    {
        Font::draw(ctx, wp + glm::vec2(24.0f, 24.0f),
            "Save Scene", glm::vec4(0.9f, 0.92f, 1.0f, 1.0f), 1.9f);

        const std::string cur = currentScenePath.empty()
            ? "(none yet)"
            : currentScenePath.filename().string();

        Font::draw(ctx, wp + glm::vec2(24.0f, 70.0f),
            "Current: " + cur, glm::vec4(0.7f), 1.3f);

        Font::draw(ctx, wp + glm::vec2(24.0f, 110.0f),
            "Overwrite current scene, or save as a new file?",
            glm::vec4(0.85f), 1.3f);

        auto btn = [&](float x, float y, float w, const char* label, Hit::Type type, glm::vec4 col)
            {
                const glm::vec2 p = wp + glm::vec2(x, y);
                const glm::vec2 s = { w, 36.0f };
                ctx.drawRect(p, s, col);
                Font::draw(ctx, p + glm::vec2(16.0f, 10.0f), label, glm::vec4(1.0f), 1.4f);
                hits.push_back({ type, -1, p, s });
            };

        btn(24.0f, 180.0f, 180.0f, "Overwrite", Hit::Type::Overwrite, { 0.25f, 0.40f, 0.65f, 1.0f });
        btn(220.0f, 180.0f, 180.0f, "Save As New", Hit::Type::SaveAsNew, { 0.20f, 0.50f, 0.35f, 1.0f });
        btn(420.0f, 180.0f, 140.0f, "Cancel", Hit::Type::Cancel, { 0.40f, 0.25f, 0.25f, 1.0f });
    }
    else if (mode == Mode::Load)
    {
        Font::draw(ctx, wp + glm::vec2(24.0f, 24.0f),
            "Load Scene", glm::vec4(0.9f, 0.92f, 1.0f, 1.0f), 1.9f);

        Font::draw(ctx, wp + glm::vec2(24.0f, 60.0f),
            "Choose a scene from this project:",
            glm::vec4(0.75f), 1.3f);

        float y = 100.0f;
        if (sceneFiles.empty())
        {
            Font::draw(ctx, wp + glm::vec2(24.0f, y),
                "No .scene files found.", glm::vec4(0.6f), 1.3f);
        }
        else
        {
            for (int i = 0; i < static_cast<int>(sceneFiles.size()); ++i)
            {
                const glm::vec2 p = wp + glm::vec2(24.0f, y);
                const glm::vec2 s = { size.x - 48.0f, 32.0f };
                ctx.drawRect(p, s, glm::vec4(0.18f, 0.20f, 0.26f, 1.0f));
                Font::draw(ctx, p + glm::vec2(12.0f, 8.0f),
                    sceneFiles[i].filename().string(), glm::vec4(0.95f), 1.35f);
                hits.push_back({ Hit::Type::SceneItem, i, p, s });
                y += 40.0f;
                if (y > size.y - 70.0f)
                    break;
            }
        }

        const glm::vec2 cancelPos = wp + glm::vec2(size.x - 160.0f, size.y - 50.0f);
        const glm::vec2 cancelSize = { 130.0f, 32.0f };
        ctx.drawRect(cancelPos, cancelSize, glm::vec4(0.40f, 0.25f, 0.25f, 1.0f));
        Font::draw(ctx, cancelPos + glm::vec2(30.0f, 8.0f), "Cancel", glm::vec4(1.0f), 1.35f);
        hits.push_back({ Hit::Type::Cancel, -1, cancelPos, cancelSize });
    }
}

bool SceneDialog::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled || mode == Mode::None)
        return false;

    if (Panel::onMouseButton(button, action, worldMousePos))
        return true;

    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return false;

    for (const auto& h : hits)
    {
        const bool inside =
            worldMousePos.x >= h.pos.x &&
            worldMousePos.x <= h.pos.x + h.size.x &&
            worldMousePos.y >= h.pos.y &&
            worldMousePos.y <= h.pos.y + h.size.y;

        if (!inside)
            continue;

        switch (h.type)
        {
        case Hit::Type::Overwrite:
            doOverwrite();
            return true;
        case Hit::Type::SaveAsNew:
            doSaveAsNew();
            return true;
        case Hit::Type::Cancel:
            close();
            return true;
        case Hit::Type::SceneItem:
            doLoad(h.index);
            return true;
        }
    }

    if (containsPoint(worldMousePos))
        return true;

    return false;
}