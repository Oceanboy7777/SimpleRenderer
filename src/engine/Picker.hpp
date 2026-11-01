#pragma once

#include "Camera.hpp"
#include "Scene.hpp"

#include <cstdint>

class Picker
{
public:
    // mouseX/Y: GLFW cursor position (top-left origin)
    // width/height: framebuffer size used for the 3D viewport
    static std::uint64_t pickObject(
        const Scene& scene,
        const Camera& camera,
        int mouseX,
        int mouseY,
        int viewportWidth,
        int viewportHeight
    );
};