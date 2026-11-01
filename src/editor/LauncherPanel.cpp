#include "LauncherPanel.hpp"
#include "ui/UIContext.hpp"
#include "ui/Font.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

LauncherPanel::LauncherPanel()
{
    setPosition({ 0.0f, 0.0f });
    setSize({ 1280.0f, 720.0f });
    setBackgroundColor({ 0.10f, 0.11f, 0.14f, 1.0f });
    refresh();
}

void LauncherPanel::setOnProjectReady(std::function<void()> cb)
{
    onProjectReady = std::move(cb);
}

void LauncherPanel::refresh()
{
    ProjectManager::ensureProjectsRoot();
    projects = ProjectManager::listProjects();
}

void LauncherPanel::createNew()
{
    // Empty name → ProjectManager names it ProjectN
    ProjectManager::createProject("");
    refresh();
    if (onProjectReady)
        onProjectReady();
}

void LauncherPanel::openAt(int index)
{
    if (index < 0 || index >= static_cast<int>(projects.size()))
        return;
    if (ProjectManager::openProject(projects[index]))
    {
        if (onProjectReady)
            onProjectReady();
    }
}

void LauncherPanel::update(UIContext& ctx, float deltaTime)
{
    Panel::update(ctx, deltaTime);
}

void LauncherPanel::draw(UIContext& ctx)
{
    if (!visible) return;

    hits.clear();

    // Full-window background
    Panel::draw(ctx);

    const glm::vec2 wp = getWorldPosition();
    const float W = size.x;

    Font::draw(ctx, wp + glm::vec2(W * 0.5f - 140.0f, 80.0f),
        "SimpleRenderer", glm::vec4(0.9f, 0.92f, 1.0f, 1.0f), 2.4f);

    Font::draw(ctx, wp + glm::vec2(W * 0.5f - 120.0f, 120.0f),
        "Project Launcher", glm::vec4(0.65f, 0.7f, 0.8f, 1.0f), 1.6f);

    // New Project button
    glm::vec2 newPos = wp + glm::vec2(W * 0.5f - 100.0f, 180.0f);
    glm::vec2 newSize = { 200.0f, 40.0f };
    ctx.drawRect(newPos, newSize, glm::vec4(0.20f, 0.45f, 0.30f, 1.0f));
    Font::draw(ctx, newPos + glm::vec2(40.0f, 12.0f), "New Project", glm::vec4(1.0f), 1.6f);
    hits.push_back({ Hit::Type::NewProject, -1, newPos, newSize });

    Font::draw(ctx, wp + glm::vec2(W * 0.5f - 100.0f, 250.0f),
        "Existing Projects", glm::vec4(0.75f), 1.4f);

    float y = 290.0f;
    for (int i = 0; i < static_cast<int>(projects.size()); ++i)
    {
        glm::vec2 rowPos = wp + glm::vec2(W * 0.5f - 180.0f, y);
        glm::vec2 rowSize = { 360.0f, 36.0f };

        ctx.drawRect(rowPos, rowSize, glm::vec4(0.16f, 0.18f, 0.22f, 1.0f));
        Font::draw(ctx, rowPos + glm::vec2(16.0f, 10.0f),
            projects[i].name + "  (Project" + std::to_string(projects[i].id) + ")",
            glm::vec4(0.95f), 1.4f);

        hits.push_back({ Hit::Type::OpenProject, i, rowPos, rowSize });
        y += 44.0f;

        if (y > size.y - 60.0f)
            break;
    }

    if (projects.empty())
    {
        Font::draw(ctx, wp + glm::vec2(W * 0.5f - 140.0f, 300.0f),
            "No projects yet. Create one.", glm::vec4(0.55f), 1.3f);
    }
}

bool LauncherPanel::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled) return false;
    if (Panel::onMouseButton(button, action, worldMousePos)) return true;
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return false;

    for (const auto& h : hits)
    {
        bool inside =
            worldMousePos.x >= h.pos.x && worldMousePos.x <= h.pos.x + h.size.x &&
            worldMousePos.y >= h.pos.y && worldMousePos.y <= h.pos.y + h.size.y;

        if (!inside) continue;

        if (h.type == Hit::Type::NewProject)
        {
            createNew();
            return true;
        }
        if (h.type == Hit::Type::OpenProject)
        {
            openAt(h.projectIndex);
            return true;
        }
    }
    return false;
}