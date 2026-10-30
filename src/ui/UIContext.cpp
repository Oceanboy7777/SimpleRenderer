#include "UIContext.hpp"
#include "Panel.hpp"
#include "Button.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

namespace
{
    const char* uiVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec4 aColor;
        layout (location = 2) in vec2 aUV;

        uniform mat4 projection;

        out vec4 vColor;
        out vec2 vUV;

        void main()
        {
            gl_Position = projection * vec4(aPos, 0.0, 1.0);
            vColor = aColor;
            vUV = aUV;
        }
    )";

        const char* uiFragmentShader = R"(
            #version 330 core
            in vec4 vColor;
            in vec2 vUV;

            uniform sampler2D uTexture;
            uniform bool uUseTexture;

            out vec4 FragColor;

            void main()
            {
                if (uUseTexture)
                {
                    float alpha = texture(uTexture, vUV).r;
                    FragColor = vec4(vColor.rgb, vColor.a * alpha);
                }
                else
                {
                    FragColor = vColor;
                }
            }
        )";
}

UIContext::UIContext()
{
    root = std::make_unique<Widget>();
}

UIContext::~UIContext()
{
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

bool UIContext::init()
{
    createShader();
    if (shaderProgram == 0)
        return false;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    // After linking the program, in init() after creating VAO/VBO:

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // position (2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    // color (4)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));

    // uv (2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);

    return true;
}

void UIContext::createShader()
{
    auto compile = [](GLenum type, const char* source) -> GLuint
        {
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);

            GLint success = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                char info[512];
                glGetShaderInfoLog(shader, 512, nullptr, info);
                std::cerr << "UI shader compile error:\n" << info << '\n';
                return 0;
            }
            return shader;
        };

    GLuint vs = compile(GL_VERTEX_SHADER, uiVertexShader);
    GLuint fs = compile(GL_FRAGMENT_SHADER, uiFragmentShader);

    if (!vs || !fs)
        return;

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    GLint success = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char info[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, info);
        std::cerr << "UI shader link error:\n" << info << '\n';
        shaderProgram = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void UIContext::beginFrame(float dt)
{
    deltaTime = dt;

    // Disable depth test for 2D UI
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Update the whole tree
    root->update(*this, deltaTime);
}

void UIContext::endFrame()
{
    // Draw the whole tree
    root->draw(*this);

    // Restore 3D state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void UIContext::drawRect(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color)
{
    if (shaderProgram == 0) return;

    const float x = pos.x;
    const float y = pos.y;
    const float w = size.x;
    const float h = size.y;

    // pos.xy, color.rgba, uv.xy
    float vertices[] = {
        x,     y,     color.r, color.g, color.b, color.a,  0.0f, 0.0f,
        x + w, y,     color.r, color.g, color.b, color.a,  1.0f, 0.0f,
        x + w, y + h, color.r, color.g, color.b, color.a,  1.0f, 1.0f,

        x,     y,     color.r, color.g, color.b, color.a,  0.0f, 0.0f,
        x + w, y + h, color.r, color.g, color.b, color.a,  1.0f, 1.0f,
        x,     y + h, color.r, color.g, color.b, color.a,  0.0f, 1.0f
    };

    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::ortho(
        0.0f, static_cast<float>(framebufferWidth),
        static_cast<float>(framebufferHeight), 0.0f,
        -1.0f, 1.0f
    );

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(shaderProgram, "uUseTexture"), 0);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void UIContext::drawTexturedRect(
    const glm::vec2& pos,
    const glm::vec2& size,
    const glm::vec4& color,
    GLuint texture,
    const glm::vec2& uvMin,
    const glm::vec2& uvMax)
{
    if (shaderProgram == 0 || texture == 0) return;

    const float x = pos.x;
    const float y = pos.y;
    const float w = size.x;
    const float h = size.y;

    float vertices[] = {
        x,     y,     color.r, color.g, color.b, color.a,  uvMin.x, uvMin.y,
        x + w, y,     color.r, color.g, color.b, color.a,  uvMax.x, uvMin.y,
        x + w, y + h, color.r, color.g, color.b, color.a,  uvMax.x, uvMax.y,

        x,     y,     color.r, color.g, color.b, color.a,  uvMin.x, uvMin.y,
        x + w, y + h, color.r, color.g, color.b, color.a,  uvMax.x, uvMax.y,
        x,     y + h, color.r, color.g, color.b, color.a,  uvMin.x, uvMax.y
    };

    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::ortho(
        0.0f, static_cast<float>(framebufferWidth),
        static_cast<float>(framebufferHeight), 0.0f,
        -1.0f, 1.0f
    );

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(shaderProgram, "uUseTexture"), 1);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void UIContext::drawText(const glm::vec2& /*pos*/, const std::string& /*text*/, const glm::vec4& /*color*/)
{
    // Placeholder – we will add real text later
}

Widget& UIContext::getRoot()
{
    return *root;
}

glm::vec2 UIContext::getMousePosition() const
{
    return mousePos;
}

bool UIContext::isMouseButtonDown(int button) const
{
    if (button < 0 || button >= 8) return false;
    return mouseButtons[button];
}

bool UIContext::isMouseOverUI() const
{
    return mouseOverUI;
}

int UIContext::getFramebufferWidth() const { return framebufferWidth; }
int UIContext::getFramebufferHeight() const { return framebufferHeight; }

void UIContext::onMouseMove(double x, double y)
{
    mousePos = { static_cast<float>(x), static_cast<float>(y) };
    mouseOverUI = root->onMouseMove(mousePos);
}

void UIContext::onMouseButton(int button, int action, int /*mods*/)
{
    if (button >= 0 && button < 8)
        mouseButtons[button] = (action == GLFW_PRESS || action == GLFW_REPEAT);

    root->onMouseButton(button, action, mousePos);
}

void UIContext::onKey(int /*key*/, int /*scancode*/, int /*action*/, int /*mods*/) {}
void UIContext::onChar(unsigned int /*codepoint*/) {}
void UIContext::onScroll(double /*xOffset*/, double /*yOffset*/) {}

void UIContext::onFramebufferSize(int width, int height)
{
    framebufferWidth = width;
    framebufferHeight = height;
}