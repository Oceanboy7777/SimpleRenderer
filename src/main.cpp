#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "engine/Camera.hpp"
#include "engine/Renderer.hpp"
#include "engine/Scene.hpp"
#include "engine/ProjectManager.hpp"
#include "engine/SceneSerializer.hpp"
#include "engine/Picker.hpp"

#include "ui/UIContext.hpp"
#include "ui/Font.hpp"

#include "editor/LauncherPanel.hpp"
#include "editor/HierarchyPanel.hpp"
#include "editor/InspectorPanel.hpp"
#include "editor/ContentBrowserPanel.hpp"
#include "editor/SceneDialog.hpp"

#include <glm/glm.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <filesystem>
#include <cmath>

namespace
{
    constexpr int windowWidth = 1280;
    constexpr int windowHeight = 720;

    UIContext* g_ui = nullptr;
    HierarchyPanel* g_hierarchy = nullptr;
    InspectorPanel* g_inspector = nullptr;
    ContentBrowserPanel* g_content = nullptr;

    enum class AppMode
    {
        Launcher,
        Editor
    };

    enum class GizmoMode
    {
        Translate,
        Rotate,
        Scale
    };

    enum class ResizeEdge
    {
        None,
        Left,
        Right,
        Bottom
    };

    struct InputState
    {
        Camera* camera = nullptr;
        Scene* scene = nullptr;
        HierarchyPanel* hierarchy = nullptr;

        AppMode* appMode = nullptr;
        bool* editMode = nullptr;

        bool firstMouse = true;
        double lastMouseX = 0.0;
        double lastMouseY = 0.0;
        bool rightMouseDown = false;

        bool fullscreen = false;
        int windowedX = 100;
        int windowedY = 100;
        int windowedW = 1280;
        int windowedH = 720;

        GizmoMode gizmoMode = GizmoMode::Translate;

        // Panel layout
        float leftW = 280.0f;
        float rightW = 280.0f;
        float bottomH = 180.0f;
        ResizeEdge resizeEdge = ResizeEdge::None;
    };

    void cursorPosCallback(GLFWwindow* window, double x, double y)
    {
        if (g_ui)
            g_ui->onMouseMove(x, y);

        auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(window));
        if (!input)
            return;

        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        const float w = static_cast<float>(fbW);
        const float h = static_cast<float>(fbH);

        if (input->resizeEdge != ResizeEdge::None)
        {
            constexpr float minSide = 180.0f;
            constexpr float minBottom = 120.0f;
            constexpr float maxSide = 500.0f;
            constexpr float maxBottom = 400.0f;

            if (input->resizeEdge == ResizeEdge::Left)
                input->leftW = std::clamp(static_cast<float>(x), minSide, maxSide);
            else if (input->resizeEdge == ResizeEdge::Right)
                input->rightW = std::clamp(w - static_cast<float>(x), minSide, maxSide);
            else if (input->resizeEdge == ResizeEdge::Bottom)
                input->bottomH = std::clamp(h - static_cast<float>(y), minBottom, maxBottom);

            return;
        }

        if (!input->camera || !input->rightMouseDown)
            return;

        if (g_ui && g_ui->isMouseOverUI())
            return;

        if (input->firstMouse)
        {
            input->lastMouseX = x;
            input->lastMouseY = y;
            input->firstMouse = false;
            return;
        }

        const float xOffset = static_cast<float>(x - input->lastMouseX);
        const float yOffset = static_cast<float>(input->lastMouseY - y);
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

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        {
            int fbW = 0, fbH = 0;
            glfwGetFramebufferSize(window, &fbW, &fbH);

            double mx = 0.0, my = 0.0;
            glfwGetCursorPos(window, &mx, &my);

            // Resize edges (editor only)
            if (input->appMode && *input->appMode == AppMode::Editor
                && input->editMode && *input->editMode)
            {
                const float w = static_cast<float>(fbW);
                const float h = static_cast<float>(fbH);
                const float x = static_cast<float>(mx);
                const float y = static_cast<float>(my);
                const float grip = 6.0f;

                input->resizeEdge = ResizeEdge::None;
                if (std::abs(x - input->leftW) <= grip && y < h - input->bottomH)
                    input->resizeEdge = ResizeEdge::Left;
                else if (std::abs(x - (w - input->rightW)) <= grip && y < h - input->bottomH)
                    input->resizeEdge = ResizeEdge::Right;
                else if (std::abs(y - (h - input->bottomH)) <= grip)
                    input->resizeEdge = ResizeEdge::Bottom;
            }

            // 3D pick (only if not over UI and not resizing)
            const bool overUI = (g_ui && g_ui->isMouseOverUI());
            if (!overUI
                && input->resizeEdge == ResizeEdge::None
                && input->appMode && *input->appMode == AppMode::Editor
                && input->editMode && *input->editMode
                && input->scene && input->camera && input->hierarchy)
            {
                const std::uint64_t id = Picker::pickObject(
                    *input->scene,
                    *input->camera,
                    static_cast<int>(mx),
                    static_cast<int>(my),
                    fbW,
                    fbH
                );

                if (id != 0)
                    input->hierarchy->setSelectedId(id);
            }
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        {
            if (input)
                input->resizeEdge = ResizeEdge::None;
        }
    }

    void scrollCallback(GLFWwindow* window, double, double yOffset)
    {
        if (g_hierarchy)
            g_hierarchy->onScroll(static_cast<float>(yOffset));

        if (g_ui && g_ui->isMouseOverUI())
            return;

        auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(window));
        if (input && input->camera)
            input->camera->processMouseScroll(static_cast<float>(yOffset));
    }

    void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
    {
        if (g_hierarchy)
            g_hierarchy->onKey(key, action);
        if (g_inspector)
            g_inspector->onKey(key, action);
        if (g_content)
            g_content->onKey(key, action);

        auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(window));
        if (!input)
            return;

        if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
        {
            if (!input->fullscreen)
            {
                glfwGetWindowPos(window, &input->windowedX, &input->windowedY);
                glfwGetWindowSize(window, &input->windowedW, &input->windowedH);
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                if (monitor && mode)
                {
                    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                    input->fullscreen = true;
                }
            }
            else
            {
                glfwSetWindowMonitor(window, nullptr, input->windowedX, input->windowedY, input->windowedW, input->windowedH, 0);
                input->fullscreen = false;
            }
        }

        if (action == GLFW_PRESS && input->appMode && *input->appMode == AppMode::Editor)
        {
            // Gizmo modes (T position, R rotate, Y scale)
            if (key == GLFW_KEY_T)
                input->gizmoMode = GizmoMode::Translate;
            else if (key == GLFW_KEY_R)
                input->gizmoMode = GizmoMode::Rotate;
            else if (key == GLFW_KEY_Y)
                input->gizmoMode = GizmoMode::Scale;
        }
    }

    void charCallback(GLFWwindow* /*window*/, unsigned int codepoint)
    {
        if (g_hierarchy)
            g_hierarchy->onChar(codepoint);

        if (g_inspector)
            g_inspector->onChar(codepoint);

        if (g_content)
            g_content->onChar(codepoint);
    }

    void framebufferSizeCallback(GLFWwindow*, int width, int height)
    {
        if (g_ui) g_ui->onFramebufferSize(width, height);
        glViewport(0, 0, width, height);
    }

    void processKeyboard(GLFWwindow* window, Camera& camera, float deltaTime, bool allowMove)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        if (!allowMove) return;
        if (g_ui && g_ui->isMouseOverUI()) return;

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

    void layoutEditorPanels(
        HierarchyPanel* hierarchy,
        InspectorPanel* inspector,
        ContentBrowserPanel* content,
        SceneDialog* dialog,
        const InputState& input,
        int width,
        int height)
    {
        const float w = static_cast<float>(width);
        const float h = static_cast<float>(height);

        const float leftW = input.leftW;
        const float rightW = input.rightW;
        const float bottomH = input.bottomH;

        hierarchy->setPosition({ 0.0f, 0.0f });
        hierarchy->setSize({ leftW, h - bottomH });

        inspector->setPosition({ w - rightW, 0.0f });
        inspector->setSize({ rightW, h - bottomH });

        content->setPosition({ 0.0f, h - bottomH });
        content->setSize({ w, bottomH });

        dialog->setPosition({ w * 0.5f - 300.0f, h * 0.5f - 180.0f });
        dialog->setSize({ 600.0f, 360.0f });
    }
}

int main()
{
    if (!glfwInit()) return EXIT_FAILURE;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "SimpleRenderer - Launcher", nullptr, nullptr);
    if (!window) { glfwTerminate(); return EXIT_FAILURE; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)))
        return EXIT_FAILURE;

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);

    UIContext ui;
    g_ui = &ui;
    if (!ui.init()) return EXIT_FAILURE;
    Font::init();
    ProjectManager::ensureProjectsRoot();

    AppMode mode = AppMode::Launcher;

    auto launcher = std::make_unique<LauncherPanel>();
    launcher->setSize({ static_cast<float>(windowWidth), static_cast<float>(windowHeight) });
    LauncherPanel* launcherPtr = launcher.get();
    ui.getRoot().addChild(std::move(launcher));

    Scene scene;
    Renderer renderer;
    Camera camera;

    auto hierarchy = std::make_unique<HierarchyPanel>();
    hierarchy->setScene(&scene);
    hierarchy->setVisible(false);

    auto inspector = std::make_unique<InspectorPanel>();
    inspector->setScene(&scene);
    inspector->setVisible(false);

    auto contentBrowser = std::make_unique<ContentBrowserPanel>();
    contentBrowser->setVisible(false);

    auto sceneDialog = std::make_unique<SceneDialog>();
    sceneDialog->setScene(&scene);
    sceneDialog->setVisible(false);

    HierarchyPanel* hierarchyPtr = hierarchy.get();
    InspectorPanel* inspectorPtr = inspector.get();
    ContentBrowserPanel* contentPtr = contentBrowser.get();
    SceneDialog* dialogPtr = sceneDialog.get();

    g_hierarchy = hierarchyPtr;
    g_inspector = inspectorPtr;
    g_content = contentPtr;
    hierarchyPtr->setSceneDialog(dialogPtr);

    ui.getRoot().addChild(std::move(hierarchy));
    ui.getRoot().addChild(std::move(inspector));
    ui.getRoot().addChild(std::move(contentBrowser));
    ui.getRoot().addChild(std::move(sceneDialog));

    launcherPtr->setOnProjectReady([&]()
        {
            mode = AppMode::Editor;
            launcherPtr->setVisible(false);
            hierarchyPtr->setVisible(true);
            inspectorPtr->setVisible(true);
            contentPtr->setScene(&scene);
            contentPtr->setVisible(true);
            contentPtr->refresh();

            auto mainScene = ProjectManager::scenesDir() / "main.scene";
            hierarchyPtr->setCurrentScenePath(mainScene);
            dialogPtr->setCurrentScenePath(mainScene);
            SceneSerializer::load(scene, mainScene.string());

            glfwSetWindowTitle(window, "SimpleRenderer - Editor");
            std::cout << "Entered EDITOR mode\n";
        });

    bool editMode = true;
    bool pWasDown = false;
    float lastTime = 0.0f;

    InputState input;
    input.camera = &camera;
    input.scene = &scene;
    input.hierarchy = hierarchyPtr;
    input.appMode = &mode;
    input.editMode = &editMode;

    glfwSetWindowUserPointer(window, &input);

    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    while (!glfwWindowShouldClose(window))
    {
        float now = static_cast<float>(glfwGetTime());
        float dt = std::min(now - lastTime, 0.1f);
        lastTime = now;

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        if (mode == AppMode::Launcher)
        {
            launcherPtr->setSize({ static_cast<float>(width), static_cast<float>(height) });
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            ui.beginFrame(dt);
            ui.endFrame();
        }
        else
        {
            static bool playWasDown = false;

            const bool altDown =
                glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

            const bool playDown =
                (altDown && glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) ||
                glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS;

            if (playDown && !playWasDown)
            {
                editMode = !editMode;
                hierarchyPtr->setVisible(editMode);
                inspectorPtr->setVisible(editMode);
                contentPtr->setVisible(editMode);
                if (!editMode)
                    dialogPtr->close();
            }
            playWasDown = playDown;

            layoutEditorPanels(hierarchyPtr, inspectorPtr, contentPtr, dialogPtr, input, width, height);

            if (!dialogPtr->getCurrentScenePath().empty())
                hierarchyPtr->setCurrentScenePath(dialogPtr->getCurrentScenePath());

            static std::uint64_t selectedForBrowser = 0;
            selectedForBrowser = hierarchyPtr->getSelectedId();
            contentPtr->setSelectedObjectId(&selectedForBrowser);

            processKeyboard(window, camera, dt, true);
            scene.update(dt);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            std::uint64_t sel = editMode ? hierarchyPtr->getSelectedId() : 0;
            if (renderer.isValid())
                renderer.render(scene, camera, width, height, selectedOutline, input.gizmoMode);

            if (editMode)
            {
                inspectorPtr->setSelectedId(hierarchyPtr->getSelectedId());
                ui.beginFrame(dt);
                ui.endFrame();
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    g_content = nullptr;
    g_hierarchy = nullptr;
    g_inspector = nullptr;
    g_ui = nullptr;

    Font::shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}