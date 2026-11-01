#pragma once

#include "ui/Panel.hpp"
#include "engine/Scene.hpp"

#include <filesystem>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace fs = std::filesystem;

class SceneDialog : public Panel
{
public:
    enum class Mode
    {
        None,
        Save,
        Load
    };

    SceneDialog();

    void setScene(Scene* scene);

    void setCurrentScenePath(const fs::path& path);
    fs::path getCurrentScenePath() const;

    void openSave(const fs::path& currentPath);
    void openLoad();
    void close();

    bool isOpen() const;
    Mode getMode() const;

    void update(UIContext& ctx, float deltaTime) override;
    void draw(UIContext& ctx) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;

private:
    struct Hit
    {
        enum class Type
        {
            Overwrite,
            SaveAsNew,
            Cancel,
            SceneItem
        } type;

        int index = -1;
        glm::vec2 pos{ 0.0f };
        glm::vec2 size{ 0.0f };
    };

    Mode mode = Mode::None;
    Scene* scene = nullptr;
    fs::path currentScenePath;
    std::vector<fs::path> sceneFiles;
    std::vector<Hit> hits;

    void doOverwrite();
    void doSaveAsNew();
    void doLoad(int index);
};