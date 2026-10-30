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

    void setPosition(const glm::vec2& pos);          // local position
    void setSize(const glm::vec2& size);
    void setVisible(bool visible);
    void setEnabled(bool enabled);

    glm::vec2 getPosition() const;                   // local
    glm::vec2 getSize() const;
    bool isVisible() const;
    bool isEnabled() const;

    // World-space helpers
    glm::vec2 getWorldPosition() const;
    bool containsPoint(const glm::vec2& worldPoint) const;

    void addChild(std::unique_ptr<Widget> child);
    const std::vector<std::unique_ptr<Widget>>& getChildren() const;

    virtual void update(UIContext& ctx, float deltaTime);
    virtual void draw(UIContext& ctx);

    virtual bool onMouseMove(const glm::vec2& worldMousePos);
    virtual bool onMouseButton(int button, int action, const glm::vec2& worldMousePos);

    // Used internally for hierarchy
    void setParent(Widget* parent);
    Widget* getParent() const;

protected:
    glm::vec2 position{ 0.0f };     // local
    glm::vec2 size{ 100.0f, 30.0f };
    bool visible = true;
    bool enabled = true;
    bool hovered = false;

    Widget* parent = nullptr;
    std::vector<std::unique_ptr<Widget>> children;
};