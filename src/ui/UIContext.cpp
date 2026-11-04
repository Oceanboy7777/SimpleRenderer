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

// ==== FONT IMPLEMENTATION ====
#include "Font.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb_truetype.h"

#include <algorithm>
#include <cstdio>

namespace font_impl
{
    GLuint texture = 0;
    bool ready = false;
    bool isTTF = false;
    int atlasW = 512;
    int atlasH = 512;
    float pixelH = 28.0f;
    constexpr int kFirst = 32;
    constexpr int kCount = 95;
    stbtt_bakedchar chars[kCount];

    bool initBitmapFallback()
    {
        // Minimal: solid white dots for every character so something shows
        atlasW = 8 * kCount;
        atlasH = 8;
        std::vector<unsigned char> pixels((size_t)atlasW * atlasH, 0);
        for (int i = 0; i < kCount; ++i)
            for (int y = 0; y < 8; ++y)
                for (int x = 1; x < 7; ++x)
                    if (y > 0 && y < 7)
                        pixels[y * atlasW + i * 8 + x] = 200;

        if (texture) glDeleteTextures(1, &texture);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasW, atlasH, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        pixelH = 8.0f;
        ready = true;
        isTTF = false;
        std::cout << "[Font] bitmap fallback\n";
        return true;
    }

    bool initTTF()
    {
        const char* paths[] = {
            "C:/SoftwareDevelopment/SimpleRenderer/assets/fonts/Inter-Regular.ttf",
            "assets/fonts/Inter-Regular.ttf",
            "../assets/fonts/Inter-Regular.ttf",
            "../../assets/fonts/Inter-Regular.ttf",
        };
        FILE* f = nullptr;
        const char* used = nullptr;
        for (const char* p : paths)
        {
#if defined(_MSC_VER)
            if (fopen_s(&f, p, "rb") == 0 && f) { used = p; break; }
#else
            f = fopen(p, "rb");
            if (f) { used = p; break; }
#endif
        }
        if (!f) return false;

        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<unsigned char> ttf((size_t)sz);
        fread(ttf.data(), 1, ttf.size(), f);
        fclose(f);
        std::cout << "[Font] Loading " << used << " (" << sz << " bytes)\n";

        int offset = stbtt_GetFontOffsetForIndex(ttf.data(), 0);
        if (offset < 0) return false;
        stbtt_fontinfo info;
        if (!stbtt_InitFont(&info, ttf.data(), offset)) return false;

        atlasW = 1024; atlasH = 1024; pixelH = 28.0f;
        std::vector<unsigned char> bitmap((size_t)atlasW * atlasH, 0);
        int rows = stbtt_BakeFontBitmap(ttf.data(), offset, pixelH,
            bitmap.data(), atlasW, atlasH, kFirst, kCount, chars);
        if (rows <= 0) {
            pixelH = 20.0f;
            std::fill(bitmap.begin(), bitmap.end(), 0);
            rows = stbtt_BakeFontBitmap(ttf.data(), offset, pixelH,
                bitmap.data(), atlasW, atlasH, kFirst, kCount, chars);
        }
        if (rows <= 0) return false;

        if (texture) glDeleteTextures(1, &texture);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasW, atlasH, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        ready = true;
        isTTF = true;
        std::cout << "[Font] Inter ready\n";
        return true;
    }
}

bool Font::init()
{
    if (font_impl::ready) return true;
    if (font_impl::initTTF()) return true;
    std::cerr << "[Font] TTF failed, using fallback\n";
    return font_impl::initBitmapFallback();
}

void Font::shutdown()
{
    if (font_impl::texture) {
        glDeleteTextures(1, &font_impl::texture);
        font_impl::texture = 0;
    }
    font_impl::ready = false;
}

void Font::draw(UIContext& ctx, const glm::vec2& pos, const std::string& text,
    const glm::vec4& color, float scale)
{
    if (!font_impl::ready || !font_impl::texture) return;

    // Map engine "scale" so 1.0 ≈ 14px UI text
    const float s = font_impl::isTTF ? scale * (14.0f / font_impl::pixelH) : scale;

    if (!font_impl::isTTF)
    {
        float x = pos.x, y = pos.y;
        float cw = 8.f * s, ch = 8.f * s;
        float aw = (float)font_impl::atlasW;
        for (unsigned char c : text) {
            if (c == '\n') { x = pos.x; y += ch + 2; continue; }
            if (c < 32 || c > 126) { x += cw; continue; }
            int idx = c - 32;
            float u0 = (idx * 8.f) / aw, u1 = ((idx + 1) * 8.f) / aw;
            ctx.drawTexturedRect({ x, y }, { cw, ch }, color, font_impl::texture, { u0, 0 }, { u1, 1 });
            x += cw;
        }
        return;
    }

    float x = 0.0f, y = 0.0f;
    const float baseline = font_impl::pixelH * 0.80f;
    for (unsigned char c : text)
    {
        if (c == '\n')
        {
            x = 0.0f;
            y += font_impl::pixelH * 1.25f;
            continue;
        }
        if (c < 32 || c > 126) continue;

        stbtt_aligned_quad q;
        float px = x, py = y;
        stbtt_GetBakedQuad(font_impl::chars, font_impl::atlasW, font_impl::atlasH,
            c - 32, &px, &py, &q, 1);

        ctx.drawTexturedRect(
            { pos.x + q.x0 * s, pos.y + (q.y0 + baseline) * s },
            { (q.x1 - q.x0) * s, (q.y1 - q.y0) * s },
            color, font_impl::texture,
            { q.s0, q.t0 }, { q.s1, q.t1 });
        x = px;
        y = py;
    }
}

float Font::getTextWidth(const std::string& text, float scale)
{
    if (!font_impl::ready || !font_impl::isTTF)
        return (float)text.size() * 8.f * scale;

    const float s = scale * (14.0f / font_impl::pixelH);
    float x = 0, y = 0, maxX = 0;
    for (unsigned char c : text)
    {
        if (c == '\n') { maxX = std::max(maxX, x); x = 0; continue; }
        if (c < 32 || c > 126) continue;
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(font_impl::chars, font_impl::atlasW, font_impl::atlasH,
            c - 32, &x, &y, &q, 1);
    }
    return std::max(maxX, x) * s;
}

float Font::getLineHeight(float scale)
{
    if (!font_impl::isTTF)
        return 8.f * scale;
    return 14.0f * 1.25f * scale; // matches normalized draw size
}