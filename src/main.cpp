#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "engine/Camera.hpp"
#include "engine/Renderer.hpp"
#include "engine/Scene.hpp"

#include "ui/UIContext.hpp"
#include "ui/Font.hpp"
#include "editor/HierarchyPanel.hpp"
#include "editor/InspectorPanel.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace
{
    constexpr int windowWidth = 1280;
    constexpr int windowHeight = 720;

    UIContext* g_ui = nullptr;

    struct InputState
    {
        Camera* camera = nullptr;
        bool firstMouse = true;
        double lastMouseX = 0.0;
        double lastMouseY = 0.0;
        bool rightMouseDown = false;
    };

    void cursorPosCallback(GLFWwindow* window, double x, double y)
    {
        if (g_ui)
            g_ui->onMouseMove(x, y);

        auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(window));
        if (!input || !input->camera)
            return;

        if (!input->rightMouseDown)
            return;

        // In edit mode, don't look while over UI
        if (g_ui && g_ui->isMouseOverUI())
            return;

        if (input->firstMouse)
        {
            input->lastMouseX = x;
            input->lastMouseY = y;
            input->firstMouse = false;
            return;
        }

        float xOffset = static_cast<float>(x - input->lastMouseX);
        float yOffset = static_cast<float>(input->lastMouseY - y);

        input->lastMouseX = x;
        input->lastMouseY = y;

        input->camera->processMouseMovement(xOffset, yOffset);
    }

    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        if (g_ui)
            g_ui->onMouseButton(button, action, mods);

        auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(window));
        if (!input)
            return;

        if (button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            if (action == GLFW_PRESS)
            {
                input->rightMouseDown = true;
                input->firstMouse = true;
            }
            else if (action == GLFW_RELEASE)
            {
                input->rightMouseDown = false;
            }
        }
    }

    void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
    {
        (void)xOffset;

        if (g_ui && g_ui->isMouseOverUI())
            return;

        auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(window));
        if (input && input->camera)
            input->camera->processMouseScroll(static_cast<float>(yOffset));
    }

    void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
    {
        if (g_ui)
            g_ui->onFramebufferSize(width, height);
        glViewport(0, 0, width, height);
    }

    void processKeyboard(GLFWwindow* window, Camera& camera, float deltaTime, bool editMode)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        // Block WASD while hovering UI only in edit mode
        if (editMode && g_ui && g_ui->isMouseOverUI())
            return;

        glm::vec3 movement(0.0f);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement += camera.getFront();
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement -= camera.getFront();
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement += camera.getRight();
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement -= camera.getRight();
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) movement.y += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) movement.y -= 1.0f;

        float len = glm::length(movement);
        if (len > 0.0f)
        {
            movement /= len;
            camera.processKeyboard(movement, deltaTime);
        }
    }
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialise GLFW\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        windowWidth, windowHeight,
        "SimpleRenderer - EDIT",
        nullptr, nullptr);

    if (!window)
    {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to load OpenGL\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);

    // ---------- UI ----------
    UIContext ui;
    g_ui = &ui;

    if (!ui.init())
    {
        std::cerr << "Failed to init UI\n";
        return EXIT_FAILURE;
    }
    Font::init();

    // ---------- Engine ----------
    Scene scene;
    Renderer renderer;
    Camera camera;

    if (!renderer.isValid())
    {
        std::cerr << "Failed to initialise renderer\n";
        return EXIT_FAILURE;
    }

    // ---------- Editor panels ----------
    auto hierarchy = std::make_unique<HierarchyPanel>();
    hierarchy->setScene(&scene);

    auto inspector = std::make_unique<InspectorPanel>();
    inspector->setScene(&scene);

    HierarchyPanel* hierarchyPtr = hierarchy.get();
    InspectorPanel* inspectorPtr = inspector.get();

    ui.getRoot().addChild(std::move(hierarchy));
    ui.getRoot().addChild(std::move(inspector));

    // ---------- Input ----------
    InputState input;
    input.camera = &camera;
    glfwSetWindowUserPointer(window, &input);

    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    std::cout << "SimpleRenderer Editor\n";
    std::cout << "P / F5  = toggle Play / Edit\n";
    std::cout << "LMB     = select in Hierarchy\n";
    std::cout << "RMB     = look around\n";
    std::cout << "WASD QE = move\n";
    std::cout << "Esc     = quit\n";

    bool editMode = true;
    bool pWasDown = false;
    float lastTime = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = std::min(currentTime - lastTime, 0.1f);
        lastTime = currentTime;

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        // Toggle Play / Edit
        bool pDown =
            glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS;

        if (pDown && !pWasDown)
        {
            editMode = !editMode;
            hierarchyPtr->setVisible(editMode);
            inspectorPtr->setVisible(editMode);
            glfwSetWindowTitle(window, editMode ? "SimpleRenderer - EDIT" : "SimpleRenderer - PLAY");
            std::cout << (editMode ? "EDIT mode\n" : "PLAY mode\n");
        }
        pWasDown = pDown;

        processKeyboard(window, camera, deltaTime, editMode);
        scene.update(deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Outline only while editing
        std::uint64_t selectedForOutline = editMode ? hierarchyPtr->getSelectedId() : 0;
        renderer.render(scene, camera, width, height, selectedForOutline);

        if (editMode)
        {
            inspectorPtr->setSelectedId(hierarchyPtr->getSelectedId());
            ui.beginFrame(deltaTime);
            ui.endFrame();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    g_ui = nullptr;
    Font::shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}