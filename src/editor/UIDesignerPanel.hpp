#pragma once

#include "ui/Panel.hpp"
#include "ui/ScrollFrame.hpp"
#include "ui/TextBox.hpp"
#include "engine/Scene.hpp"
#include "engine/GameObject.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <glm/glm.hpp>

// Full-screen UI Designer. Hierarchy only shows "Interface".
// Add: Text, TextBox, ScrollFrame, Frame, CheckBox, Button.
class UIDesignerPanel : public Panel
{
public:
    UIDesignerPanel();

    void setScene(Scene* scene);
    void setUIRootId(std::uint64_t id);
    std::uint64_t getUIRootId() const;

    void setOnClose(std::function<void()> cb);
    void setOnSave(std::function<void()> cb);
    void setContentBrowser(class ContentBrowserPanel* browser);

    void update(UIContext& ctx, float deltaTime) override;
    void draw(UIContext& ctx) override;
    bool onMouseMove(const glm::vec2& worldMousePos) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;
    void onKey(int key, int action);
    void onChar(unsigned int codepoint);
    void onScroll(float yOffset);

    std::uint64_t getSelectedElementId() const { return selectedElementId; }

private:
    Scene* scene = nullptr;
    std::uint64_t uiRootId = 0;
    std::uint64_t selectedElementId = 0;
    std::uint64_t hoveredElementId = 0;

    std::function<void()> onClose;
    std::function<void()> onSave;
    class ContentBrowserPanel* contentBrowser = nullptr;

    bool createMenuOpen = false;

    glm::vec2 canvasPos{ 0.0f };
    glm::vec2 canvasSize{ 0.0f };

    bool dragging = false;
    std::uint64_t dragId = 0;
    glm::vec2 dragOffset{ 0.0f };

    TextBox nameBox;
    TextBox zOrderBox;
    TextBox textBox;
    TextBox colRBox, colGBox, colBBox;

    ScrollFrame hierarchyScroll;
    float hierarchyContentH = 0.0f;

    struct Hit
    {
        enum class Type
        {
            Close,
            Save,
            AddElement,
            CreateText,
            CreateTextBox,
            CreateScrollFrame,
            CreateFrame,
            CreateCheckBox,
            CreateButton,
            SelectElement
        } type;
        glm::vec2 pos{ 0.0f };
        glm::vec2 size{ 0.0f };
        std::uint64_t id = 0;
    };
    std::vector<Hit> hits;

    // Cached button rects (rebuilt every draw, used next frame for clicks)
    glm::vec2 closePos{ 0.0f }, closeSize{ 0.0f };
    glm::vec2 plusPos{ 0.0f }, plusSize{ 0.0f };
    glm::vec2 menuPos{ 0.0f }, menuSize{ 0.0f };

    void syncInspectorFromSelection();
    void applyInspector();
    void createElement(UIType type, const char* baseName);
    void drawElementOnCanvas(UIContext& ctx, const GameObject& obj);
    bool hitTest(const glm::vec2& p, const glm::vec2& pos, const glm::vec2& size) const;
};
