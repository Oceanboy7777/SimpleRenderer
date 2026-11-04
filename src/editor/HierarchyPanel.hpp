#pragma once

#include "ui/Panel.hpp"
#include "ui/ScrollFrame.hpp"
#include "ui/TextBox.hpp"

#include "engine/Scene.hpp"
#include "engine/GameObject.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include <glm/glm.hpp>

class SceneDialog;

class HierarchyPanel : public Panel
{
public:
    HierarchyPanel();

    void setScene(Scene* scene);
    void setSceneDialog(SceneDialog* dialog);
    void setCurrentScenePath(const std::filesystem::path& path);
    std::filesystem::path getCurrentScenePath() const;

    void setSelectedId(std::uint64_t id);
    std::uint64_t getSelectedId() const;

    // Called when user wants to open the UI Designer for a UIRoot
    void setOnOpenUIDesigner(std::function<void(std::uint64_t uiRootId)> cb);

    void update(UIContext& ctx, float deltaTime) override;
    void draw(UIContext& ctx) override;
    bool onMouseMove(const glm::vec2& worldMousePos) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;

    void onKey(int key, int action);
    void onChar(unsigned int codepoint);
    void onScroll(float yOffset);

private:
    enum class Category
    {
        Gamespace = 0,
        UserInterface,
        Rendering,
        SoundService,
        PhysicsService,
        ScriptService,
        Count
    };

    struct CategoryState
    {
        const char* name = nullptr;
        bool expanded = true;
    };

    struct ItemRect
    {
        std::uint64_t id = 0;
        glm::vec2 pos{ 0.0f };
        glm::vec2 size{ 0.0f };
        bool isCategory = false;
        int categoryIndex = -1;
        bool isUIRoot = false;
    };

    struct ActionHit
    {
        enum class Type
        {
            OpenCreateMenu,
            CreateStaticCube,
            CreateStaticPlane,
            CreateStaticPyramid,
            CreateStaticCustom,
            CreateStaticSphere,
            CreateStaticCone,
            ToggleStaticMeshMenu,
            CreateNewUserInterface,
            OpenUIDesigner,
            SaveScene,
            LoadScene,
            Delete
        } type;

        glm::vec2 pos{ 0.0f };
        glm::vec2 size{ 0.0f };
        int categoryIndex = -1;
        std::uint64_t targetId = 0;
    };

    Scene* scene = nullptr;
    SceneDialog* sceneDialog = nullptr;
    std::filesystem::path currentScenePath;

    std::uint64_t selectedId = 0;
    std::uint64_t hoveredId = 0;

    CategoryState categories[static_cast<int>(Category::Count)];
    std::vector<ItemRect> itemRects;
    std::vector<ActionHit> actionHits;

    bool createMenuOpen = false;
    bool staticMeshMenuOpen = false;
    int createMenuCategory = -1;

    glm::vec2 lastCreateMenuPos{ 0.0f };
    glm::vec2 lastCreateMenuSize{ 0.0f };
    glm::vec2 lastCreateButtonPos{ 0.0f };

    TextBox searchBox;
    std::string searchFilter;

    bool renaming = false;
    std::uint64_t renameId = 0;
    std::string renameBuffer;

    ScrollFrame listScroll;
    float listContentHeight = 0.0f;

    std::function<void(std::uint64_t)> onOpenUIDesigner;

    void initCategories();
    void createStaticMesh(MeshType type, const std::string& name);
    void createNewUserInterface();
    void beginRename();
    void commitRename();
    void cancelRename();
    void saveScene();
    void loadScene();
    void deleteSelected();
};
