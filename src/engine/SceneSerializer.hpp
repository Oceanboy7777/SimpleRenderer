#pragma once

#include "Scene.hpp"
#include <string>

class SceneSerializer
{
public:
    // Returns true on success
    static bool save(const Scene& scene, const std::string& path);
    static bool load(Scene& scene, const std::string& path);
};