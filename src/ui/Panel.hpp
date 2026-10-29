#pragma once

#include "Widget.hpp"
#include <glm/glm.hpp>

class Panel : public Widget
{
public:
    Panel();

    void setBackgroundColor(const glm::vec4& color);
    void setBorderColor(const glm::vec4& color);
    void setBorderThickness(float thickness);

    void draw(UIContext& ctx) override;

private:
    glm::vec4 backgroundColor{ 0.15f, 0.16f, 0.20f, 0.95f };
    glm::vec4 borderColor{ 0.30f, 0.32f, 0.38f, 1.0f };
    float borderThickness = 1.0f;
};