#pragma once

#include "Widget.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

class UIContext
{
public:
    UIContext();
    ~UIContext();

    // Call once after OpenGL is ready
    bool init();

    // Call at the start of every frame
    void beginFrame(float deltaTime);

    // Call after all UI drawing
    void endFrame();

    // Input (call these from your GLFW callbacks)
    void onMouseMove(double x, double y);
    void onMouseButton(int button, int action, int mods);
    void onKey(int key, int scancode, int action, int mods);
    void onChar(unsigned int codepoint);
    void onScroll(double xOffset, double yOffset);
    void onFramebufferSize(int width, int height);

    // Drawing helpers (used by widgets)
    void drawRect(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color);
    void drawText(const glm::vec2& pos, const std::string& text, const glm::vec4& color);

    // Root widget access
    Widget& getRoot();

    // Current mouse state
    glm::vec2 getMousePosition() const;
    bool isMouseButtonDown(int button) const;

    int getFramebufferWidth() const;
    int getFramebufferHeight() const;

private:
    void createShader();
    void createFontTexture(); // simple placeholder for now

    std::unique_ptr<Widget> root;

    // OpenGL resources
    GLuint shaderProgram = 0;
    GLuint vao = 0;
    GLuint vbo = 0;

    // State
    glm::vec2 mousePos{ 0.0f };
    bool mouseButtons[8] = {};
    int framebufferWidth = 1280;
    int framebufferHeight = 720;

    float deltaTime = 0.0f;
};