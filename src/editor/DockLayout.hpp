#pragma once

#include "ui/UIContext.hpp"
#include <glm/glm.hpp>
#include <array>
#include <string>

// Four dock slots around a central 3D viewport (UE-style).
// Panels can be dragged by their title bar and dropped onto another slot to swap.

enum class DockSlot : int
{
    Left = 0,
    Right = 1,
    Bottom = 2,
    Center = 3,
    Count = 4
};

enum class DockPanelId : int
{
    Hierarchy = 0,
    Inspector = 1,
    ContentBrowser = 2,
    Viewport = 3,
    Count = 4
};

struct DockRect
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    bool contains(float px, float py) const
    {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

class DockLayout
{
public:
    static constexpr float kTitleH = 22.0f;
    static constexpr float kSplitGrip = 5.0f;

    DockLayout();

    // Split sizes (pixels)
    float leftW = 280.0f;
    float rightW = 280.0f;
    float bottomH = 180.0f;

    void setWindowSize(float width, float height, float topBarH);
    void recompute();

    DockRect slotRect(DockSlot slot) const;
    DockRect panelBodyRect(DockPanelId id) const; // content area under title bar
    DockRect panelTitleRect(DockPanelId id) const;
    DockRect viewportRect() const; // full center slot including title

    DockPanelId panelInSlot(DockSlot slot) const;
    DockSlot slotOf(DockPanelId id) const;

    // Input — call from main mouse handling (editor mode only)
    // Returns true if event was consumed (dragging / resizing splits)
    bool onMouseButton(float mx, float my, int button, bool pressed);
    bool onMouseMove(float mx, float my);

    void drawChrome(UIContext& ui) const; // title bars + drop highlights

    bool isDragging() const { return dragPanel_ != DockPanelId::Count; }

private:
    float winW_ = 1280.0f;
    float winH_ = 720.0f;
    float topBarH_ = 28.0f;

    std::array<DockPanelId, 4> slots_{
        DockPanelId::Hierarchy,
        DockPanelId::Inspector,
        DockPanelId::ContentBrowser,
        DockPanelId::Viewport
    };

    DockPanelId dragPanel_ = DockPanelId::Count;
    DockSlot hoverSlot_ = DockSlot::Count;
    float dragOffsetX_ = 0.0f;

    enum class Split { None, Left, Right, Bottom } activeSplit_ = Split::None;

    static const char* panelTitle(DockPanelId id);
    void swapSlots(DockSlot a, DockSlot b);
};
