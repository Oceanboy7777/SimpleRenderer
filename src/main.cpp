
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "engine/Camera.hpp"
#include "engine/Renderer.hpp"
#include "engine/Scene.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace
{
    constexpr int windowWidth = 1280;
    constexpr int windowHeight = 720;

    struct InputState
    {
        Camera* camera = nullptr;
        bool firstMouseMovement = true;
        double lastMouseX = 0.0;
        double lastMouseY = 0.0;
    };

    void cursorPositionCallback(
        GLFWwindow* window,
        double mouseX,
        double mouseY
    )
    {
        auto* input = static_cast<InputState*>(
            glfwGetWindowUserPointer(window)
            );

        if (input == nullptr || input->camera == nullptr)
        {
            return;
        }

        if (input->firstMouseMovement)
        {
            input->lastMouseX = mouseX;
            input->lastMouseY = mouseY;
            input->firstMouseMovement = false;
            return;
        }

        const float xOffset = static_cast<float>(
            mouseX - input->lastMouseX
            );

        const float yOffset = static_cast<float>(
            input->lastMouseY - mouseY
            );

        input->lastMouseX = mouseX;
        input->lastMouseY = mouseY;

        input->camera->processMouseMovement(xOffset, yOffset);
    }

    void scrollCallback(
        GLFWwindow* window,
        double xOffset,
        double yOffset
    )
    {
        (void)xOffset;

        auto* input = static_cast<InputState*>(
            glfwGetWindowUserPointer(window)
            );

        if (input != nullptr && input->camera != nullptr)
        {
            input->camera->processMouseScroll(
                static_cast<float>(yOffset)
            );
        }
    }

    void processInput(
        GLFWwindow* window,
        Camera& camera,
        float deltaTime
    )
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        glm::vec3 movement(0.0f);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            movement += camera.getFront();
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            movement -= camera.getFront();
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            movement += camera.getRight();
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            movement -= camera.getRight();
        }

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        {
            movement.y += 1.0f;
        }

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        {
            movement.y -= 1.0f;
        }

        // Normalise diagonal movement so it isn't faster.
        const float movementLength = glm::length(movement);

        if (movementLength > 0.0f)
        {
            movement /= movementLength;
            camera.processKeyboard(movement, deltaTime);
        }
    }
}

int main()
{
    if (glfwInit() != GLFW_TRUE)
    {
        std::cerr << "Failed to initialise GLFW.\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    GLFWwindow* window = glfwCreateWindow(
        windowWidth,
        windowHeight,
        "SimpleRenderer - Milestone 8",
        nullptr,
        nullptr
    );

    if (window == nullptr)
    {
        std::cerr << "Failed to create the application window.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (gladLoadGL(
        reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)
    ) == 0)
    {
        std::cerr << "Failed to load OpenGL functions.\n";

        glfwDestroyWindow(window);
        glfwTerminate();

        return EXIT_FAILURE;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);

    int framebufferWidth = 0;
    int framebufferHeight = 0;

    glfwGetFramebufferSize(
        window,
        &framebufferWidth,
        &framebufferHeight
    );

    glViewport(0, 0, framebufferWidth, framebufferHeight);

    Camera camera;

    InputState input;
    input.camera = &camera;

    glfwSetWindowUserPointer(window, &input);

    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // Capture the mouse for first-person camera control.
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    int result = EXIT_SUCCESS;

    // Keep engine resources inside this scope.
    // They are destroyed before the OpenGL context is destroyed.
    {
        Scene scene;
        Renderer renderer;

        if (!renderer.isValid())
        {
            std::cerr << "Failed to initialise the renderer.\n";
            result = EXIT_FAILURE;
        }
        else
        {
            std::cout << "SimpleRenderer Milestone 8 started.\n";
            std::cout << "WASD: move | Mouse: look | E/Q: up/down\n";
            std::cout << "Mouse wheel: adjust field of view | Esc: exit\n";

            float lastFrameTime = 0.0f;

            while (glfwWindowShouldClose(window) == GLFW_FALSE)
            {
                const float currentFrameTime =
                    static_cast<float>(glfwGetTime());

                const float deltaTime = std::min(
                    currentFrameTime - lastFrameTime,
                    0.1f
                );

                lastFrameTime = currentFrameTime;

                processInput(window, camera, deltaTime);
                scene.update(deltaTime);

                glfwGetFramebufferSize(
                    window,
                    &framebufferWidth,
                    &framebufferHeight
                );

                renderer.render(
                    scene,
                    camera,
                    framebufferWidth,
                    framebufferHeight
                );

                glfwSwapBuffers(window);
                glfwPollEvents();
            }
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return result;
}
