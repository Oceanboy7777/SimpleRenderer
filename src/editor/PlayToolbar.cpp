#include "PlayToolbar.hpp"
#include "ui/UIContext.hpp"
#include "ui/Font.hpp"

#include <GLFW/glfw3.h>

PlayToolbar::PlayToolbar()
{
    setSize({ 140.0f, 36.0f });
    setBackgroundColor({ 0.12f, 0.13f, 0.16f, 0.95f });
}

void PlayToolbar::setPlaying(bool p)
{
    playing = p;
}

bool PlayToolbar::isPlaying() const
{
    return playing;
}

void PlayToolbar::setOnPlay(std::function<void()> cb)
{
    onPlay = std::move(cb);
}

void PlayToolbar::setOnStop(std::function<void()> cb)
{
    onStop = std::move(cb);
}

void PlayToolbar::draw(UIContext& ctx)
{
    if (!visible)
        return;

    hits.clear();
    Panel::draw(ctx);

    const glm::vec2 wp = getWorldPosition();

    auto btn = [&](float x, const char* label, Action action, glm::vec4 col)
        {
            glm::vec2 p = wp + glm::vec2(x, 6.0f);
            glm::vec2 s = { 60.0f, 24.0f };
            ctx.drawRect(p, s, col);
            const float tw = Font::getTextWidth(label, 1.2f);
            Font::draw(ctx, p + glm::vec2((s.x - tw) * 0.5f, 5.0f),
                label, glm::vec4(1.0f), 1.2f);
            hits.push_back({ action, p, s });
        };

    btn(8.0f, "Play", Action::Play,
        playing ? glm::vec4(0.15f, 0.25f, 0.18f, 1.0f)
        : glm::vec4(0.20f, 0.50f, 0.30f, 1.0f));

    btn(72.0f, "Stop", Action::Stop,
        !playing ? glm::vec4(0.25f, 0.15f, 0.15f, 1.0f)
        : glm::vec4(0.55f, 0.22f, 0.22f, 1.0f));
}

bool PlayToolbar::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled)
        return false;

    if (Panel::onMouseButton(button, action, worldMousePos))
        return true;

    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return false;

    for (const auto& h : hits)
    {
        const bool inside =
            worldMousePos.x >= h.pos.x && worldMousePos.x <= h.pos.x + h.size.x &&
            worldMousePos.y >= h.pos.y && worldMousePos.y <= h.pos.y + h.size.y;

        if (!inside)
            continue;

        if (h.action == Action::Play && onPlay)
            onPlay();
        if (h.action == Action::Stop && onStop)
            onStop();
        return true;
    }

    return containsPoint(worldMousePos);
}