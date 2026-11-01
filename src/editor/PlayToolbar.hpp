#pragma once

#include "ui/Panel.hpp"

#include <functional>
#include <vector>
#include <glm/glm.hpp>

class PlayToolbar : public Panel
{
public:
    PlayToolbar();

    void setPlaying(bool playing);
    bool isPlaying() const;

    void setOnPlay(std::function<void()> cb);
    void setOnStop(std::function<void()> cb);

    void draw(UIContext& ctx) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;

private:
    enum class Action
    {
        Play,
        Stop
    };

    struct Hit
    {
        Action action = Action::Play;
        glm::vec2 pos{ 0.0f };
        glm::vec2 size{ 0.0f };
    };

    bool playing = false;
    std::vector<Hit> hits;
    std::function<void()> onPlay;
    std::function<void()> onStop;
};