#define NOMINMAX
#include "DockLayout.hpp"
#include "ui/Font.hpp"
#include <algorithm>
#include <cmath>

DockLayout::DockLayout() = default;

void DockLayout::setWindowSize(float width, float height, float topBarH)
{
    winW_ = width;
    winH_ = height;
    topBarH_ = topBarH;
}

void DockLayout::recompute()
{
    const float minSide = 160.0f;
    const float maxSide = (std::max)(minSide, winW_ * 0.45f);
    const float minBottom = 100.0f;
    const float maxBottom = (std::max)(minBottom, winH_ * 0.5f);

    leftW = std::clamp(leftW, minSide, maxSide);
    rightW = std::clamp(rightW, minSide, maxSide);
    bottomH = std::clamp(bottomH, minBottom, maxBottom);

    // Ensure center still has room
    if (leftW + rightW > winW_ - 120.0f)
    {
        const float t = leftW + rightW;
        leftW = leftW / t * (winW_ - 120.0f);
        rightW = rightW / t * (winW_ - 120.0f);
    }
    if (bottomH > winH_ - topBarH_ - 80.0f)
        bottomH = winH_ - topBarH_ - 80.0f;
}

DockRect DockLayout::slotRect(DockSlot slot) const
{
    const float y0 = topBarH_;
    const float midH = winH_ - topBarH_ - bottomH;
    DockRect r{};
    switch (slot)
    {
    case DockSlot::Left:
        r = { 0.0f, y0, leftW, midH };
        break;
    case DockSlot::Right:
        r = { winW_ - rightW, y0, rightW, midH };
        break;
    case DockSlot::Bottom:
        r = { 0.0f, winH_ - bottomH, winW_, bottomH };
        break;
    case DockSlot::Center:
        r = { leftW, y0, winW_ - leftW - rightW, midH };
        break;
    default:
        break;
    }
    return r;
}

DockPanelId DockLayout::panelInSlot(DockSlot slot) const
{
    const int i = static_cast<int>(slot);
    if (i < 0 || i >= 4) return DockPanelId::Count;
    return slots_[i];
}

DockSlot DockLayout::slotOf(DockPanelId id) const
{
    for (int i = 0; i < 4; ++i)
        if (slots_[i] == id)
            return static_cast<DockSlot>(i);
    return DockSlot::Count;
}

DockRect DockLayout::panelTitleRect(DockPanelId id) const
{
    const DockSlot s = slotOf(id);
    DockRect r = slotRect(s);
    r.h = kTitleH;
    return r;
}

DockRect DockLayout::panelBodyRect(DockPanelId id) const
{
    const DockSlot s = slotOf(id);
    DockRect r = slotRect(s);
    r.y += kTitleH;
    r.h = (std::max)(0.0f, r.h - kTitleH);
    return r;
}

DockRect DockLayout::viewportRect() const
{
    return slotRect(slotOf(DockPanelId::Viewport));
}

const char* DockLayout::panelTitle(DockPanelId id)
{
    switch (id)
    {
    case DockPanelId::Hierarchy: return "Hierarchy";
    case DockPanelId::Inspector: return "Inspector";
    case DockPanelId::ContentBrowser: return "Content Browser";
    case DockPanelId::Viewport: return "Viewport";
    default: return "";
    }
}

void DockLayout::swapSlots(DockSlot a, DockSlot b)
{
    if (a == b || a == DockSlot::Count || b == DockSlot::Count) return;
    std::swap(slots_[static_cast<int>(a)], slots_[static_cast<int>(b)]);
}

bool DockLayout::onMouseButton(float mx, float my, int button, bool pressed)
{
    // button: 0 = left
    if (button != 0)
        return false;

    if (!pressed)
    {
        if (dragPanel_ != DockPanelId::Count)
        {
            if (hoverSlot_ != DockSlot::Count)
            {
                const DockSlot from = slotOf(dragPanel_);
                if (from != hoverSlot_)
                    swapSlots(from, hoverSlot_);
            }
            dragPanel_ = DockPanelId::Count;
            hoverSlot_ = DockSlot::Count;
            return true;
        }
        if (activeSplit_ != Split::None)
        {
            activeSplit_ = Split::None;
            return true;
        }
        return false;
    }

    // Press
    // Split grips first
    const float y0 = topBarH_;
    const float midH = winH_ - topBarH_ - bottomH;
    if (my >= y0 && my <= y0 + midH)
    {
        if (std::abs(mx - leftW) <= kSplitGrip)
        {
            activeSplit_ = Split::Left;
            return true;
        }
        if (std::abs(mx - (winW_ - rightW)) <= kSplitGrip)
        {
            activeSplit_ = Split::Right;
            return true;
        }
    }
    if (std::abs(my - (winH_ - bottomH)) <= kSplitGrip && my > topBarH_)
    {
        activeSplit_ = Split::Bottom;
        return true;
    }

    // Title bars — start drag
    for (int i = 0; i < 4; ++i)
    {
        const DockPanelId id = static_cast<DockPanelId>(i);
        const DockRect title = panelTitleRect(id);
        if (title.contains(mx, my))
        {
            dragPanel_ = id;
            hoverSlot_ = slotOf(id);
            dragOffsetX_ = mx - title.x;
            return true;
        }
    }
    return false;
}

bool DockLayout::onMouseMove(float mx, float my)
{
    if (activeSplit_ != Split::None)
    {
        if (activeSplit_ == Split::Left)
            leftW = mx;
        else if (activeSplit_ == Split::Right)
            rightW = winW_ - mx;
        else if (activeSplit_ == Split::Bottom)
            bottomH = winH_ - my;
        recompute();
        return true;
    }

    if (dragPanel_ != DockPanelId::Count)
    {
        hoverSlot_ = DockSlot::Count;
        for (int i = 0; i < 4; ++i)
        {
            const DockSlot s = static_cast<DockSlot>(i);
            if (slotRect(s).contains(mx, my))
            {
                hoverSlot_ = s;
                break;
            }
        }
        return true;
    }
    return false;
}

void DockLayout::drawChrome(UIContext& ui) const
{
    for (int i = 0; i < 4; ++i)
    {
        const DockPanelId id = static_cast<DockPanelId>(i);
        const DockRect title = panelTitleRect(id);
        const bool dragging = (dragPanel_ == id);
        const DockSlot s = slotOf(id);
        const bool dropTarget = (dragPanel_ != DockPanelId::Count && hoverSlot_ == s && dragPanel_ != id);

        // Title bar background
        glm::vec4 bg = dragging
            ? glm::vec4(0.18f, 0.32f, 0.55f, 1.0f)
            : glm::vec4(0.12f, 0.14f, 0.18f, 1.0f);
        if (dropTarget)
            bg = glm::vec4(0.15f, 0.35f, 0.55f, 1.0f);

        ui.drawRect({ title.x, title.y }, { title.w, title.h }, bg);

        // Thin blue accent line at top of each frame
        ui.drawRect({ title.x, title.y }, { title.w, 2.0f }, glm::vec4(0.25f, 0.55f, 0.95f, 1.0f));

        Font::draw(ui, { title.x + 8.0f, title.y + 5.0f }, panelTitle(id),
                   glm::vec4(0.92f, 0.93f, 0.96f, 1.0f), 1.0f);

        // Frame border (subtle)
        const DockRect full = slotRect(s);
        const glm::vec4 border(0.08f, 0.09f, 0.11f, 1.0f);
        ui.drawRect({ full.x, full.y }, { full.w, 1.0f }, border);
        ui.drawRect({ full.x, full.y + full.h - 1.0f }, { full.w, 1.0f }, border);
        ui.drawRect({ full.x, full.y }, { 1.0f, full.h }, border);
        ui.drawRect({ full.x + full.w - 1.0f, full.y }, { 1.0f, full.h }, border);
    }

    // Drop zone overlay while dragging
    if (dragPanel_ != DockPanelId::Count && hoverSlot_ != DockSlot::Count)
    {
        const DockRect r = slotRect(hoverSlot_);
        ui.drawRect({ r.x + 2.0f, r.y + 2.0f }, { r.w - 4.0f, r.h - 4.0f },
                    glm::vec4(0.20f, 0.45f, 0.85f, 0.18f));
    }
}
