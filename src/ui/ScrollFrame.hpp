#pragma once

#include "Widget.hpp"
#include <glm/glm.hpp>

class ScrollFrame : public Widget
{
public:
    ScrollFrame();

    void setContentHeight(float height);
    float getScrollY() const;
    void setScrollY(float y);

    // Convert a local Y (inside content) to world Y after scroll
    float contentToWorldY(float localY) const;

    void draw(UIContext& ctx) override;
    bool onMouseMove(const glm::vec2& worldMousePos) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;

    // Call from main when wheel moves and mouse is over this frame
    void onScroll(float yOffset);

private:
    float contentHeight = 0.0f;
    float scrollY = 0.0f;
    bool hovered = false;

    float maxScroll() const;
};