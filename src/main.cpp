#define NOMINMAX
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "engine/Camera.hpp"
#include "engine/Renderer.hpp"
#include "engine/Scene.hpp"
#include "engine/ProjectManager.hpp"
#include "engine/SceneSerializer.hpp"
#include "engine/Picker.hpp"
#include "engine/GameObject.hpp"

#include "ui/UIContext.hpp"
#include "ui/Font.hpp"

#include "editor/GizmoMode.hpp"
#include "editor/LauncherPanel.hpp"
#include "editor/HierarchyPanel.hpp"
#include "editor/InspectorPanel.hpp"
#include "editor/ContentBrowserPanel.hpp"
#include "editor/SceneDialog.hpp"
#include "editor/TransformToolbar.hpp"
#include "editor/PlayToolbar.hpp"
#include "editor/UIDesignerPanel.hpp"
#include "editor/DockLayout.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>
#include <memory>

namespace
{
    constexpr int windowWidth = 1280;
    constexpr int windowHeight = 720;

    UIContext* g_ui = nullptr;
    HierarchyPanel* g_hierarchy = nullptr;
    InspectorPanel* g_inspector = nullptr;
    ContentBrowserPanel* g_content = nullptr;
    UIDesignerPanel* g_uiDesigner = nullptr;

    enum class AppMode
    {
        Launcher,
        Editor,
        UIDesigner
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
        TransformToolbar* transformToolbar = nullptr;

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

        float leftW = 280.0f;
        float rightW = 280.0f;
        float bottomH = 180.0f;
        DockLayout* dock = nullptr;
        ResizeEdge resizeEdge = ResizeEdge::None;

        // Gizmo drag
        bool gizmoDragging = false;
        int gizmoAxis = -1;
        glm::vec3 gizmoDragStartObj{ 0.0f };
        glm::vec3 gizmoDragStartHit{ 0.0f };
        glm::vec3 gizmoDragStartRot{ 0.0f };
        glm::vec3 gizmoDragStartScale{ 1.0f };
        float gizmoDragStartMouseX = 0.0f;
        float gizmoDragStartMouseY = 0.0f;
    };

    bool rayPlane(
        const glm::vec3& ro,
        const glm::vec3& rd,
        const glm::vec3& planePoint,
        const glm::vec3& planeN,
        glm::vec3& outHit)
    {
        const float denom = glm::dot(planeN, rd);
        if (std::abs(denom) < 1e-6f)
            return false;
        const float t = glm::dot(planePoint - ro, planeN) / denom;
        if (t < 0.0f)
            return false;
        outHit = ro + rd * t;
        return true;
    }

    void screenToRay(
        const Camera& camera,
        int mx,
        int my,
        int w,
        int h,
        glm::vec3& origin,
        glm::vec3& dir)
    {
        const float ndcX = (2.0f * static_cast<float>(mx)) / static_cast<float>((std::max)(w, 1)) - 1.0f;
        const float ndcY = 1.0f - (2.0f * static_cast<float>(my)) / static_cast<float>((std::max)(h, 1));
        const float aspect = static_cast<float>(w) / static_cast<float>((std::max)(h, 1));

        const glm::mat4 invProj = glm::inverse(glm::perspective(
            glm::radians(camera.getFieldOfView()), aspect, 0.1f, 500.0f));
        const glm::mat4 invView = glm::inverse(camera.getViewMatrix());

        glm::vec4 eye = invProj * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        eye = glm::vec4(eye.x, eye.y, -1.0f, 0.0f);
        dir = glm::normalize(glm::vec3(invView * eye));
        origin = camera.getPosition();
    }

    int pickGizmoAxis(
        const Camera& camera,
        const glm::vec3& objPos,
        int mx,
        int my,
        int w,
        int h)
    {
        glm::vec3 ro;
        glm::vec3 rd;
        screenToRay(camera, mx, my, w, h, ro, rd);

        const float axisLen = 1.25f;
        const float hitR = 0.22f;
        float bestT = 1e9f;
        int best = -1;

        auto testAxis = [&](int axis, const glm::vec3& aDir)
            {
                const glm::vec3 a0 = objPos;
                const glm::vec3 a1 = objPos + aDir * axisLen;
                const glm::vec3 u = rd;
                const glm::vec3 v = a1 - a0;
                const glm::vec3 w0 = ro - a0;
                const float a = glm::dot(u, u);
                const float b = glm::dot(u, v);
                const float c = glm::dot(v, v);
                const float d = glm::dot(u, w0);
                const float e = glm::dot(v, w0);
                const float denom = a * c - b * b;
                float t = 0.0f;
                float s = 0.0f;
                if (std::abs(denom) < 1e-8f)
                {
                    t = 0.0f;
                    s = (c > 1e-8f) ? e / c : 0.0f;
                }
                else
                {
                    t = (b * e - c * d) / denom;
                    s = (a * e - b * d) / denom;
                }
                s = std::clamp(s, 0.0f, 1.0f);
                if (t < 0.0f)
                    return;

                const glm::vec3 pRay = ro + u * t;
                const glm::vec3 pAxis = a0 + v * s;
                const float dist = glm::length(pRay - pAxis);
                if (dist < hitR && t < bestT)
                {
                    bestT = t;
                    best = axis;
                }
            };

        testAxis(0, { 1.0f, 0.0f, 0.0f });
        testAxis(1, { 0.0f, 1.0f, 0.0f });
        testAxis(2, { 0.0f, 0.0f, 1.0f });
        return best;
    }

    void cursorPosCallback(GLFWwindow* window, double x, double y)
    {
        if (g_ui)
            g_ui->onMouseMove(x, y);

        auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(window));
        if (!input)
            return;

        int fbW = 0;
        int fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        const float w = static_cast<float>(fbW);
        const float h = static_cast<float>(fbH);

        if (input->dock && input->appMode && *input->appMode == AppMode::Editor)
        {
            if (input->dock->onMouseMove(static_cast<float>(x), static_cast<float>(y)))
            {
                input->leftW = input->dock->leftW;
                input->rightW = input->dock->rightW;
                input->bottomH = input->dock->bottomH;
                return;
            }
        }

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

        if (input->gizmoDragging
            && input->scene
            && input->camera
            && input->hierarchy
            && input->gizmoAxis >= 0)
        {
            const std::uint64_t sel = input->hierarchy->getSelectedId();
            GameObject* obj = input->scene->findObjectById(sel);
            if (obj)
            {
                Transform& t = obj->getTransform();

                if (input->gizmoMode == GizmoMode::Translate)
                {
                    glm::vec3 ro;
                    glm::vec3 rd;
                    int gx = static_cast<int>(x), gy = static_cast<int>(y), gw = fbW, gh = fbH;
                    if (input->dock)
                    {
                        const DockRect vp = input->dock->viewportRect();
                        gx = static_cast<int>(x - vp.x);
                        gy = static_cast<int>(y - vp.y);
                        gw = (std::max)(1, static_cast<int>(vp.w));
                        gh = (std::max)(1, static_cast<int>(vp.h));
                    }
                    screenToRay(*input->camera, gx, gy, gw, gh, ro, rd);

                    glm::vec3 axisDir(0.0f);
                    axisDir[input->gizmoAxis] = 1.0f;
                    glm::vec3 planeN = glm::cross(axisDir, glm::cross(input->camera->getFront(), axisDir));
                    if (glm::length(planeN) < 1e-4f)
                        planeN = glm::vec3(0.0f, 1.0f, 0.0f);
                    else
                        planeN = glm::normalize(planeN);

                    glm::vec3 hit;
                    if (rayPlane(ro, rd, input->gizmoDragStartObj, planeN, hit))
                    {
                        const glm::vec3 delta = hit - input->gizmoDragStartHit;
                        const float along = glm::dot(delta, axisDir);
                        glm::vec3 newPos = input->gizmoDragStartObj;
                        newPos[input->gizmoAxis] = input->gizmoDragStartObj[input->gizmoAxis] + along;
                        t.setPosition(newPos);
                    }
                }
                else if (input->gizmoMode == GizmoMode::Rotate)
                {
                    const float dx = static_cast<float>(x) - input->gizmoDragStartMouseX;
                    glm::vec3 rot = input->gizmoDragStartRot;
                    rot[input->gizmoAxis] += dx * 0.4f;
                    t.setRotation(rot);
                }
                else if (input->gizmoMode == GizmoMode::Scale)
                {
                    const float dy = input->gizmoDragStartMouseY - static_cast<float>(y);
                    glm::vec3 scale = input->gizmoDragStartScale;
                    scale[input->gizmoAxis] = (std::max)(0.05f, input->gizmoDragStartScale[input->gizmoAxis] + dy * 0.01f);
                    t.setScale(scale);
                }

                if (g_inspector)
                    g_inspector->setSelectedId(sel);
            }
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

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && input->dock)
        {
            double mx = 0.0, my = 0.0;
            glfwGetCursorPos(window, &mx, &my);
            if (input->dock->onMouseButton(static_cast<float>(mx), static_cast<float>(my), 0, false))
            {
                input->leftW = input->dock->leftW;
                input->rightW = input->dock->rightW;
                input->bottomH = input->dock->bottomH;
            }
            input->resizeEdge = ResizeEdge::None;
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        {
            int fbW = 0;
            int fbH = 0;
            glfwGetFramebufferSize(window, &fbW, &fbH);

            double mx = 0.0;
            double my = 0.0;
            glfwGetCursorPos(window, &mx, &my);

            if (input->appMode && *input->appMode == AppMode::Editor
                && input->editMode && *input->editMode && input->dock)
            {
                const float xf = static_cast<float>(mx);
                const float yf = static_cast<float>(my);

                // Dock title-bar drag / split grips handled by DockLayout
                if (input->dock->onMouseButton(xf, yf, 0, true))
                {
                    input->resizeEdge = ResizeEdge::None;
                    // Sync sizes back
                    input->leftW = input->dock->leftW;
                    input->rightW = input->dock->rightW;
                    input->bottomH = input->dock->bottomH;
                }
                else
                {
                    input->resizeEdge = ResizeEdge::None;
                }
            }

            const bool overUI = (g_ui && g_ui->isMouseOverUI());
            // Prefer viewport hit-testing when dock is active
            bool overViewport = true;
            int pickX = static_cast<int>(mx);
            int pickY = static_cast<int>(my);
            int pickW = fbW;
            int pickH = fbH;
            if (input->dock && input->appMode && *input->appMode == AppMode::Editor)
            {
                const DockRect vp = input->dock->viewportRect();
                overViewport = vp.contains(static_cast<float>(mx), static_cast<float>(my));
                pickX = static_cast<int>(mx - vp.x);
                pickY = static_cast<int>(my - vp.y);
                pickW = (std::max)(1, static_cast<int>(vp.w));
                pickH = (std::max)(1, static_cast<int>(vp.h));
            }

            if (!overUI
                && overViewport
                && input->resizeEdge == ResizeEdge::None
                && input->appMode && *input->appMode == AppMode::Editor
                && input->editMode && *input->editMode
                && input->scene && input->camera && input->hierarchy
                && !(input->dock && input->dock->isDragging()))
            {
                const std::uint64_t sel = input->hierarchy->getSelectedId();
                if (sel != 0)
                {
                    GameObject* obj = input->scene->findObjectById(sel);
                    if (obj)
                    {
                        const int axis = pickGizmoAxis(
                            *input->camera,
                            obj->getPosition(),
                            pickX,
                            pickY,
                            pickW,
                            pickH);

                        if (axis >= 0)
                        {
                            input->gizmoDragging = true;
                            input->gizmoAxis = axis;
                            input->gizmoDragStartObj = obj->getPosition();
                            input->gizmoDragStartRot = obj->getTransform().getRotation();
                            input->gizmoDragStartScale = obj->getScale();
                            input->gizmoDragStartMouseX = static_cast<float>(mx);
                            input->gizmoDragStartMouseY = static_cast<float>(my);

                            glm::vec3 ro;
                            glm::vec3 rd;
                            screenToRay(
                                *input->camera,
                                pickX,
                                pickY,
                                pickW,
                                pickH,
                                ro,
                                rd);

                            glm::vec3 axisDir(0.0f);
                            axisDir[axis] = 1.0f;
                            glm::vec3 planeN = glm::cross(axisDir, glm::cross(input->camera->getFront(), axisDir));
                            if (glm::length(planeN) < 1e-4f)
                                planeN = glm::vec3(0.0f, 1.0f, 0.0f);
                            else
                                planeN = glm::normalize(planeN);

                            glm::vec3 hit;
                            if (rayPlane(ro, rd, obj->getPosition(), planeN, hit))
                                input->gizmoDragStartHit = hit;
                            else
                                input->gizmoDragStartHit = obj->getPosition();

                            return;
                        }
                    }
                }

                const std::uint64_t id = Picker::pickObject(
                    *input->scene,
                    *input->camera,
                    pickX,
                    pickY,
                    pickW,
                    pickH);

                // Click empty space → clear selection
                input->hierarchy->setSelectedId(id);
            }
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        {
            input->resizeEdge = ResizeEdge::None;
            input->gizmoDragging = false;
            input->gizmoAxis = -1;
        }
    }

    void scrollCallback(GLFWwindow* window, double /*xOffset*/, double yOffset)
    {
        if (g_uiDesigner && g_uiDesigner->isVisible())
        {
            g_uiDesigner->onScroll(static_cast<float>(yOffset));
            return;
        }
        if (g_hierarchy)
            g_hierarchy->onScroll(static_cast<float>(yOffset));
        if (g_content)
            g_content->onScroll(static_cast<float>(yOffset));

        if (g_ui && g_ui->isMouseOverUI())
            return;

        auto* input = static_cast<InputState*>(glfwGetWindowUserPointer(window));
        if (input && input->camera)
            input->camera->processMouseScroll(static_cast<float>(yOffset));
    }

    void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
    {
        if (g_uiDesigner && g_uiDesigner->isVisible())
        {
            g_uiDesigner->onKey(key, action);
            return;
        }
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
                    glfwSetWindowMonitor(
                        window, monitor, 0, 0,
                        mode->width, mode->height, mode->refreshRate);
                    input->fullscreen = true;
                }
            }
            else
            {
                glfwSetWindowMonitor(
                    window, nullptr,
                    input->windowedX, input->windowedY,
                    input->windowedW, input->windowedH, 0);
                input->fullscreen = false;
            }
        }

        // R = position, T = rotation, Y = scale
        if (action == GLFW_PRESS && input->appMode && *input->appMode == AppMode::Editor)
        {
            if (key == GLFW_KEY_R)
            {
                input->gizmoMode = GizmoMode::Translate;
                if (input->transformToolbar)
                    input->transformToolbar->setMode(GizmoMode::Translate);
            }
            else if (key == GLFW_KEY_T)
            {
                input->gizmoMode = GizmoMode::Rotate;
                if (input->transformToolbar)
                    input->transformToolbar->setMode(GizmoMode::Rotate);
            }
            else if (key == GLFW_KEY_Y)
            {
                input->gizmoMode = GizmoMode::Scale;
                if (input->transformToolbar)
                    input->transformToolbar->setMode(GizmoMode::Scale);
            }
        }
    }

    void charCallback(GLFWwindow* /*window*/, unsigned int codepoint)
    {
        if (g_uiDesigner && g_uiDesigner->isVisible())
        {
            g_uiDesigner->onChar(codepoint);
            return;
        }
        if (g_hierarchy)
            g_hierarchy->onChar(codepoint);
        if (g_inspector)
            g_inspector->onChar(codepoint);
        if (g_content)
            g_content->onChar(codepoint);
    }

    void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
    {
        if (g_ui)
            g_ui->onFramebufferSize(width, height);
        glViewport(0, 0, width, height);
    }

    void processKeyboard(GLFWwindow* window, Camera& camera, float deltaTime, bool allowMove)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        if (!allowMove)
            return;
        if (g_ui && g_ui->isMouseOverUI())
            return;

        glm::vec3 movement(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement += camera.getFront();
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement -= camera.getFront();
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement += camera.getRight();
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement -= camera.getRight();
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) movement.y += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) movement.y -= 1.0f;

        const float len = glm::length(movement);
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
        TransformToolbar* transformTb,
        PlayToolbar* playTb,
        DockLayout& dock,
        int width,
        int height)
    {
        constexpr float tabBarH = 28.0f;
        const float w = static_cast<float>(width);
        const float h = static_cast<float>(height);

        dock.setWindowSize(w, h, tabBarH);
        dock.recompute();

        auto place = [&](DockPanelId id, auto* panel)
        {
            if (!panel) return;
            const DockRect body = dock.panelBodyRect(id);
            panel->setPosition({ body.x, body.y });
            panel->setSize({ body.w, body.h });
        };

        place(DockPanelId::Hierarchy, hierarchy);
        place(DockPanelId::Inspector, inspector);
        place(DockPanelId::ContentBrowser, content);

        dialog->setPosition({ w * 0.5f - 300.0f, h * 0.5f - 180.0f });
        dialog->setSize({ 600.0f, 360.0f });

        // Toolbars float inside the viewport slot
        const DockRect vp = dock.viewportRect();
        transformTb->setPosition({ vp.x + vp.w - 176.0f, vp.y + DockLayout::kTitleH + 6.0f });
        transformTb->setSize({ 168.0f, 36.0f });

        playTb->setPosition({ vp.x + vp.w * 0.5f - 70.0f, vp.y + DockLayout::kTitleH + 6.0f });
        playTb->setSize({ 140.0f, 36.0f });
    }
}

int main()
{
    if (!glfwInit())
        return EXIT_FAILURE;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        windowWidth, windowHeight, "SimpleRenderer - Launcher", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)))
        return EXIT_FAILURE;

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);

    UIContext ui;
    g_ui = &ui;
    if (!ui.init())
        return EXIT_FAILURE;

    Font::init();
    ProjectManager::ensureProjectsRoot();

    AppMode mode = AppMode::Launcher;
    bool editMode = true;

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

    auto transformToolbar = std::make_unique<TransformToolbar>();
    transformToolbar->setVisible(false);

    auto playToolbar = std::make_unique<PlayToolbar>();
    playToolbar->setVisible(false);

    auto uiDesigner = std::make_unique<UIDesignerPanel>();
    uiDesigner->setVisible(false);   // must start hidden
    UIDesignerPanel* uiDesignerPtr = uiDesigner.get();
    g_uiDesigner = uiDesignerPtr;

    HierarchyPanel* hierarchyPtr = hierarchy.get();
    InspectorPanel* inspectorPtr = inspector.get();
    ContentBrowserPanel* contentPtr = contentBrowser.get();
    SceneDialog* dialogPtr = sceneDialog.get();
    TransformToolbar* transformTb = transformToolbar.get();
    PlayToolbar* playTb = playToolbar.get();

    g_hierarchy = hierarchyPtr;
    g_inspector = inspectorPtr;
    g_content = contentPtr;
    hierarchyPtr->setSceneDialog(dialogPtr);

    hierarchyPtr->setOnOpenUIDesigner([&](std::uint64_t rootId)
        {
            mode = AppMode::UIDesigner;
            uiDesignerPtr->setScene(&scene);
            uiDesignerPtr->setUIRootId(rootId);
            uiDesignerPtr->setVisible(true);

            hierarchyPtr->setVisible(false);
            inspectorPtr->setVisible(false);
            contentPtr->setVisible(false);
            transformTb->setVisible(false);
            playTb->setVisible(false);
            launcherPtr->setVisible(false);

            uiDesignerPtr->setOnSave([&]()
                {
                    const auto path = hierarchyPtr->getCurrentScenePath();
                    if (!path.empty())
                    {
                        SceneSerializer::save(scene, path.string());
                        std::cout << "UI Designer: saved scene\n";
                    }
                });
            uiDesignerPtr->setContentBrowser(contentPtr);
            uiDesignerPtr->setOnClose([&]()
                {
                    // Close removes this UI root from the scene (and thus from the tab bar)
                    const std::uint64_t rid = uiDesignerPtr->getUIRootId();
                    if (rid != 0)
                        scene.removeObject(rid);

                    mode = AppMode::Editor;
                    uiDesignerPtr->setVisible(false);
                    hierarchyPtr->setVisible(true);
                    inspectorPtr->setVisible(true);
                    contentPtr->setVisible(true);
                    transformTb->setVisible(true);
                    playTb->setVisible(true);
                    glfwSetWindowTitle(window, "SimpleRenderer - Editor");
                });

            glfwSetWindowTitle(window, "SimpleRenderer - UI Designer");
        });

    // Content & inspector first; hierarchy last so its dropdowns draw on top
    ui.getRoot().addChild(std::move(contentBrowser));
    ui.getRoot().addChild(std::move(inspector));
    ui.getRoot().addChild(std::move(hierarchy));
    ui.getRoot().addChild(std::move(sceneDialog));
    ui.getRoot().addChild(std::move(transformToolbar));
    ui.getRoot().addChild(std::move(playToolbar));
    ui.getRoot().addChild(std::move(uiDesigner));

    InputState input;
    input.camera = &camera;
    input.scene = &scene;
    input.hierarchy = hierarchyPtr;
    input.transformToolbar = transformTb;
    input.appMode = &mode;
    input.editMode = &editMode;

    DockLayout dock;
    input.dock = &dock;
    // Sync initial split sizes
    dock.leftW = input.leftW;
    dock.rightW = input.rightW;
    dock.bottomH = input.bottomH;

    glfwSetWindowUserPointer(window, &input);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    launcherPtr->setOnProjectReady([&]()
        {
            mode = AppMode::Editor;
            launcherPtr->setVisible(false);
            hierarchyPtr->setVisible(true);
            inspectorPtr->setVisible(true);
            contentPtr->setScene(&scene);
            contentPtr->setVisible(true);
            contentPtr->refresh();
            transformTb->setVisible(true);
            playTb->setVisible(true);
            playTb->setPlaying(false);
            editMode = true;

            const auto mainScene = ProjectManager::scenesDir() / "main.scene";
            hierarchyPtr->setCurrentScenePath(mainScene);
            dialogPtr->setCurrentScenePath(mainScene);
            SceneSerializer::load(scene, mainScene.string());

            glfwSetWindowTitle(window, "SimpleRenderer - Editor");
            std::cout << "Entered EDITOR mode\n";
        });

    playTb->setOnPlay([&]()
        {
            editMode = false;
            hierarchyPtr->setVisible(false);
            inspectorPtr->setVisible(false);
            contentPtr->setVisible(false);
            transformTb->setVisible(false);
            playTb->setPlaying(true);
            dialogPtr->close();
        });

    playTb->setOnStop([&]()
        {
            editMode = true;
            hierarchyPtr->setVisible(true);
            inspectorPtr->setVisible(true);
            contentPtr->setVisible(true);
            transformTb->setVisible(true);
            playTb->setPlaying(false);
        });

    transformTb->setOnModeChanged([&](GizmoMode m)
        {
            input.gizmoMode = m;
        });

    bool playWasDown = false;
    float lastTime = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        const float now = static_cast<float>(glfwGetTime());
        const float dt = (std::min)(now - lastTime, 0.1f);
        lastTime = now;

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        if (mode == AppMode::Launcher)
        {
            // Ensure designer stays hidden on launcher
            if (uiDesignerPtr) uiDesignerPtr->setVisible(false);

            launcherPtr->setVisible(true);
            launcherPtr->setSize({ static_cast<float>(width), static_cast<float>(height) });
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            ui.beginFrame(dt);
            ui.endFrame();
        }
        else if (mode == AppMode::UIDesigner)
        {
            constexpr float tabBarH = 28.0f;
            const float w = static_cast<float>(width);
            const float h = static_cast<float>(height);

            launcherPtr->setVisible(false);
            hierarchyPtr->setVisible(false);
            inspectorPtr->setVisible(false);
            transformTb->setVisible(false);
            playTb->setVisible(false);

            // Designer fills under the tab bar; content browser is laid out by designer
            uiDesignerPtr->setPosition({ 0.0f, tabBarH });
            uiDesignerPtr->setSize({ w, h - tabBarH });
            uiDesignerPtr->setVisible(true);
            uiDesignerPtr->setContentBrowser(contentPtr);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            ui.beginFrame(dt);

            // Tab bar always visible
            {
                ui.drawRect({ 0.0f, 0.0f }, { w, tabBarH }, glm::vec4(0.11f, 0.12f, 0.15f, 1.0f));
                struct TabHit { std::uint64_t id; float x0, x1; };
                static std::vector<TabHit> tabHits;
                tabHits.clear();
                float tx = 6.0f;
                auto drawTab = [&](const std::string& label, bool active, std::uint64_t id)
                {
                    const float pad = 14.0f;
                    const float tw = Font::getTextWidth(label, 1.15f) + pad * 2.0f;
                    glm::vec2 tp = { tx, 2.0f };
                    glm::vec2 ts = { tw, tabBarH - 4.0f };
                    ui.drawRect(tp, ts, active ? glm::vec4(0.22f, 0.38f, 0.72f, 1.0f)
                                               : glm::vec4(0.16f, 0.17f, 0.21f, 1.0f));
                    Font::draw(ui, tp + glm::vec2(pad, 6.0f), label, glm::vec4(0.95f), 1.15f);
                    tabHits.push_back({ id, tx, tx + tw });
                    tx += tw + 4.0f;
                };
                drawTab("Main Engine", false, 0);
                for (const auto& op : scene.getObjects())
                {
                    if (op->isUIRoot())
                        drawTab(op->getName(), op->getId() == uiDesignerPtr->getUIRootId(), op->getId());
                }

                static bool wasDown = false;
                const bool down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
                if (down && !wasDown)
                {
                    double mx = 0.0, my = 0.0;
                    glfwGetCursorPos(window, &mx, &my);
                    if (my >= 0.0 && my <= tabBarH)
                    {
                        for (const auto& tab : tabHits)
                        {
                            if (mx >= tab.x0 && mx <= tab.x1)
                            {
                                if (tab.id == 0)
                                {
                                    mode = AppMode::Editor;
                                    uiDesignerPtr->setVisible(false);
                                    hierarchyPtr->setVisible(true);
                                    inspectorPtr->setVisible(true);
                                    contentPtr->setVisible(true);
                                    transformTb->setVisible(true);
                                    playTb->setVisible(true);
                                    glfwSetWindowTitle(window, "SimpleRenderer - Editor");
                                }
                                else if (tab.id != uiDesignerPtr->getUIRootId())
                                {
                                    uiDesignerPtr->setUIRootId(tab.id);
                                }
                                break;
                            }
                        }
                    }
                }
                wasDown = down;
            }

            ui.endFrame();
        }
        else
        {
            // Editor mode — keep designer hidden
            if (uiDesignerPtr) uiDesignerPtr->setVisible(false);

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
                transformTb->setVisible(editMode);
                playTb->setPlaying(!editMode);
                if (!editMode)
                    dialogPtr->close();
            }
            playWasDown = playDown;

            // Keep dock splits in sync with input (resize grips update dock)
            dock.leftW = input.leftW;
            dock.rightW = input.rightW;
            dock.bottomH = input.bottomH;

            if (!editMode)
            {
                // Play mode: viewport fills the client area under the tab bar
                dock.leftW = 0.0f;
                dock.rightW = 0.0f;
                dock.bottomH = 0.0f;
            }

            layoutEditorPanels(
                hierarchyPtr, inspectorPtr, contentPtr, dialogPtr,
                transformTb, playTb, dock, width, height);

            if (editMode)
            {
                input.leftW = dock.leftW;
                input.rightW = dock.rightW;
                input.bottomH = dock.bottomH;
            }

            playTb->setVisible(true);
            transformTb->setVisible(editMode);

            if (!dialogPtr->getCurrentScenePath().empty())
                hierarchyPtr->setCurrentScenePath(dialogPtr->getCurrentScenePath());

            static std::uint64_t selectedForBrowser = 0;
            selectedForBrowser = hierarchyPtr->getSelectedId();
            contentPtr->setSelectedObjectId(&selectedForBrowser);

            processKeyboard(window, camera, dt, true);
            scene.update(dt);

            // Full window clear (panel chrome background)
            glDisable(GL_SCISSOR_TEST);
            glViewport(0, 0, width, height);
            glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            const std::uint64_t selectedId =
                editMode ? hierarchyPtr->getSelectedId() : 0;

            // 3D only in the Viewport dock slot
            const DockRect vp = dock.viewportRect();
            const int vpx = static_cast<int>(vp.x);
            const int vpy = static_cast<int>(vp.y);
            const int vpw = (std::max)(1, static_cast<int>(vp.w));
            const int vph = (std::max)(1, static_cast<int>(vp.h));
            // OpenGL origin is bottom-left
            const int glY = height - vpy - vph;

            if (renderer.isValid() && vpw > 1 && vph > 1)
            {
                glEnable(GL_SCISSOR_TEST);
                glScissor(vpx, glY, vpw, vph);
                glViewport(vpx, glY, vpw, vph);
                glClearColor(0.10f, 0.11f, 0.14f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                renderer.render(
                    scene,
                    camera,
                    vpw,
                    vph,
                    selectedId,
                    input.gizmoMode,
                    editMode
                );

                glDisable(GL_SCISSOR_TEST);
                glViewport(0, 0, width, height);
            }

            if (editMode)
                inspectorPtr->setSelectedId(hierarchyPtr->getSelectedId());

            // ---- Top tab bar: Main Engine + one tab per UI root ----
            struct TabHit { std::uint64_t id; float x0, x1; }; // id==0 => Main Engine
            static std::vector<TabHit> tabHits;
            tabHits.clear();

            ui.beginFrame(dt);

            // Dock title bars + drop highlights (under panels' own draw which already ran in endFrame... 
            // beginFrame updates widgets; we draw chrome here before endFrame draws the tree)
            if (editMode)
                dock.drawChrome(ui);

            {
                const float tabBarH = 28.0f;
                const float w = static_cast<float>(width);
                ui.drawRect({ 0.0f, 0.0f }, { w, tabBarH }, glm::vec4(0.11f, 0.12f, 0.15f, 1.0f));

                float tx = 6.0f;
                auto drawTab = [&](const std::string& label, bool active, std::uint64_t id)
                {
                    const float pad = 14.0f;
                    const float tw = Font::getTextWidth(label, 1.15f) + pad * 2.0f;
                    const float th = tabBarH - 4.0f;
                    glm::vec2 tp = { tx, 2.0f };
                    glm::vec2 ts = { tw, th };
                    if (active)
                        ui.drawRect(tp, ts, glm::vec4(0.22f, 0.38f, 0.72f, 1.0f));
                    else
                        ui.drawRect(tp, ts, glm::vec4(0.16f, 0.17f, 0.21f, 1.0f));
                    Font::draw(ui, tp + glm::vec2(pad, 6.0f), label, glm::vec4(0.95f), 1.15f);
                    tabHits.push_back({ id, tx, tx + tw });
                    tx += tw + 4.0f;
                };

                drawTab("Main Engine", true, 0); // currently in editor = main active

                if (input.scene)
                {
                    for (const auto& op : input.scene->getObjects())
                    {
                        if (op->isUIRoot())
                            drawTab(op->getName(), false, op->getId());
                    }
                }

                // Click handling for tabs (poll left mouse just pressed)
                static bool wasDown = false;
                const bool down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
                if (down && !wasDown)
                {
                    double mx = 0.0, my = 0.0;
                    glfwGetCursorPos(window, &mx, &my);
                    if (my >= 0.0 && my <= tabBarH)
                    {
                        for (const auto& t : tabHits)
                        {
                            if (mx >= t.x0 && mx <= t.x1)
                            {
                                if (t.id == 0)
                                {
                                    // already on main engine
                                }
                                else
                                {
                                    // open that UI root in designer
                                    mode = AppMode::UIDesigner;
                                    uiDesignerPtr->setScene(&scene);
                                    uiDesignerPtr->setUIRootId(t.id);
                                    uiDesignerPtr->setVisible(true);
                                    hierarchyPtr->setVisible(false);
                                    inspectorPtr->setVisible(false);
                                    contentPtr->setVisible(false);
                                    transformTb->setVisible(false);
                                    playTb->setVisible(false);
                                    uiDesignerPtr->setContentBrowser(contentPtr);
                                    uiDesignerPtr->setOnSave([&]()
                                    {
                                        const auto path = hierarchyPtr->getCurrentScenePath();
                                        if (!path.empty())
                                            SceneSerializer::save(scene, path.string());
                                    });
                                    uiDesignerPtr->setOnClose([&]()
                                    {
                                        const std::uint64_t rid = uiDesignerPtr->getUIRootId();
                                        if (rid != 0)
                                            scene.removeObject(rid);
                                        mode = AppMode::Editor;
                                        uiDesignerPtr->setVisible(false);
                                        hierarchyPtr->setVisible(true);
                                        inspectorPtr->setVisible(true);
                                        contentPtr->setVisible(true);
                                        transformTb->setVisible(true);
                                        playTb->setVisible(true);
                                        glfwSetWindowTitle(window, "SimpleRenderer - Editor");
                                    });
                                    glfwSetWindowTitle(window, "SimpleRenderer - UI Designer");
                                }
                                break;
                            }
                        }
                    }
                }
                wasDown = down;
            }

            ui.endFrame();
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