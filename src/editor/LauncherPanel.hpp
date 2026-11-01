#pragma once

#include "ui/Panel.hpp"
#include "engine/ProjectManager.hpp"
#include <vector>
#include <string>
#include <functional>
#include <glm/glm.hpp>

class LauncherPanel : public Panel
{
public:
    LauncherPanel();

    // Called when user opens or creates a project
    void setOnProjectReady(std::function<void()> cb);

    void refresh();
    void update(UIContext& ctx, float deltaTime) override;
    void draw(UIContext& ctx) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;

private:
    struct Hit
    {
        enum class Type { NewProject, OpenProject } type;
        int projectIndex = -1;
        glm::vec2 pos{ 0.0f };
        glm::vec2 size{ 0.0f };
    };

    std::vector<ProjectInfo> projects;
    std::vector<Hit> hits;
    std::function<void()> onProjectReady;

    // Simple name buffer for "New Project" (no text field yet — uses default name)
    void createNew();
    void openAt(int index);
};