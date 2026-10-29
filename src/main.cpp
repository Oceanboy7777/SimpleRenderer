#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "engine/Camera.hpp"
#include "engine/Renderer.hpp"
#include "engine/Scene.hpp"
#include "ui/UIContext.hpp"
#include "ui/Panel.hpp"
#include "ui/Button.hpp"

#include <iostream>
#include <memory>

namespace
{
    UIContext* g_ui = nullptr;

    void cursorPosCallback(GLFWwindow* window, double x, double y)
    {
        if (g_ui) g_ui->onMouseMove(x, y);
    }

    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        if (g_ui) g_ui->onMouseButton(button, action, mods);
    }

    void framebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        if (g_ui) g_ui->onFramebufferSize(width, height);
        glViewport(0, 0, width, height);
    }
}

int main()
{
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "SimpleRenderer - UI Test", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
    {
        std::cerr << "Failed to load OpenGL\n";
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);

    // --- UI setup ---
    UIContext ui;
    g_ui = &ui;

    if (!ui.init())
    {
        std::cerr << "Failed to init UI\n";
        return 1;
    }

    // Create a test panel
    auto panel = std::make_unique<Panel>();
    panel->setPosition({ 50.0f, 50.0f });
    panel->setSize({ 280.0f, 400.0f });

    // Add a button inside the panel
    auto button = std::make_unique<Button>();
    button->setPosition({ 70.0f, 80.0f });   // relative to screen for now
    button->setSize({ 140.0f, 36.0f });
    button->setLabel("Click Me");
    button->setOnClick([]() {
        std::cout << "Button clicked!\n";
        });

    panel->addChild(std::move(button));
    ui.getRoot().addChild(std::move(panel));

    // Callbacks
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Show the mouse cursor (important for UI)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    Scene scene;
    Renderer renderer;
    Camera camera;

    float lastTime = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 3D scene
        if (renderer.isValid())
        {
            renderer.render(scene, camera, width, height);
        }

        // UI on top
        ui.beginFrame(deltaTime);
        ui.endFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    g_ui = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}