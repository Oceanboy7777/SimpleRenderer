#pragma once

#include "ui/Panel.hpp"
#include "editor/GizmoMode.hpp"

#include <functional>
#include <vector>
#include <glm/glm.hpp>

class TransformToolbar : public Panel
{
public:
    TransformToolbar();

    void setMode(GizmoMode mode);
    GizmoMode getMode() const;
    void setOnModeChanged(std::function<void(GizmoMode)> cb);

    void draw(UIContext& ctx) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;

private:
    struct Hit
    {
        GizmoMode mode = GizmoMode::Translate;
        glm::vec2 pos{ 0.0f };
        glm::vec2 size{ 0.0f };
    };

    GizmoMode mode = GizmoMode::Translate;
    std::vector<Hit> hits;
    std::function<void(GizmoMode)> onModeChanged;
};