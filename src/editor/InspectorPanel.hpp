#pragma once

#include "ui/Panel.hpp"
#include "ui/TextBox.hpp"
#include "engine/Scene.hpp"
#include "engine/GameObject.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>

class InspectorPanel : public Panel
{
public:
    InspectorPanel();

    void setScene(Scene* scene);
    void setSelectedId(std::uint64_t id);

    void update(UIContext& ctx, float deltaTime) override;
    void draw(UIContext& ctx) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;

    void onKey(int key, int action);
    void onChar(unsigned int codepoint);

private:
    enum class Field
    {
        PosX, PosY, PosZ,
        ScaleX, ScaleY, ScaleZ,
        RotX, RotY, RotZ,
        ColR, ColG, ColB,
        Count
    };

    struct MeshHit
    {
        std::string path;
        glm::vec2 pos{ 0.0f };
        glm::vec2 size{ 0.0f };
    };

    Scene* scene = nullptr;
    std::uint64_t selectedId = 0;
    std::uint64_t lastSyncedId = 0;

    TextBox fields[static_cast<int>(Field::Count)];
    std::vector<MeshHit> meshHits;

    void setupFields();
    void syncFieldsFromObject();
    void applyField(Field field, const std::string& text);
    void layoutAndDrawFields(UIContext& ctx, float& y);
    void drawMeshSection(UIContext& ctx, float& y, GameObject* obj);
    static float parseFloat(const std::string& text, float fallback);
};