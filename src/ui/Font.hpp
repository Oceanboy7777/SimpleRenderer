#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>

class UIContext;

class Font
{
public:
    static bool init();
    static void shutdown();

    // Draws text at world position (top-left of first character)
    static void draw(UIContext& ctx, const glm::vec2& pos, const std::string& text, const glm::vec4& color, float scale = 1.0f);

    static float getTextWidth(const std::string& text, float scale = 1.0f);
    static float getLineHeight(float scale = 1.0f);

private:
    static GLuint texture;
    static bool initialized;
};