#pragma once

#include "Scene.hpp"
#include <string>

class SceneSerializer
{
public:
    // Uses SIMPLERENDERER_ASSET_DIR/scenes/main.scene by default
    static std::string defaultScenePath();

    static bool save(const Scene& scene, const std::string& path = "");
    static bool load(Scene& scene, const std::string& path = "");
};