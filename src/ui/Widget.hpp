#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
#include <functional>

class UIContext;

class Widget
{
public:
    Widget();
    virtual ~Widget() = default;

    void setPosition(const glm::vec2& pos);
    void setSize(const glm::vec2& size);
    void setVisible(bool visible);
    void setEnabled(bool enabled);

    glm::vec2 getPosition() const;
    glm::vec2 getSize() const;
    bool isVisible() const;
    bool isEnabled() const;

    // Hierarchy
    void addChild(std::unique_ptr<Widget> child);
    const std::vector<std::unique_ptr<Widget>>& getChildren() const;

    // Called every frame
    virtual void update(UIContext& ctx, float deltaTime);
    virtual void draw(UIContext& ctx);

    // Input
    virtual bool onMouseMove(const glm::vec2& mousePos);
    virtual bool onMouseButton(int button, int action, const glm::vec2& mousePos);

protected:
    glm::vec2 position{ 0.0f };
    glm::vec2 size{ 100.0f, 30.0f };
    bool visible = true;
    bool enabled = true;
    bool hovered = false;

    std::vector<std::unique_ptr<Widget>> children;
};