
#include "Texture.hpp"

#include <filesystem>
#include <iostream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifndef SIMPLERENDERER_ASSET_DIR
#define SIMPLERENDERER_ASSET_DIR "assets"
#endif

Texture::Texture(
    const std::string& filePath,
    bool useCheckerFallback
)
    : textureId(0),
    loadedFromFile(false)
{
    if (!filePath.empty())
    {
        loadedFromFile = loadFromFile(filePath);
    }

    if (!loadedFromFile && useCheckerFallback)
    {
        createCheckerTexture();
    }
}

Texture::~Texture()
{
    if (textureId != 0)
    {
        glDeleteTextures(1, &textureId);
    }
}

bool Texture::isValid() const
{
    return textureId != 0;
}

void Texture::bind(unsigned int textureUnit) const
{
    if (textureId == 0)
    {
        return;
    }

    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, textureId);
}

GLuint Texture::getId() const
{
    return textureId;
}

bool Texture::wasLoadedFromFile() const
{
    return loadedFromFile;
}



bool Texture::loadFromFile(const std::string& filePath)
{
    int width = 0;
    int height = 0;
    int channels = 0;

    const std::filesystem::path absoluteTexturePath =
        std::filesystem::path(SIMPLERENDERER_ASSET_DIR) /
        std::filesystem::path(filePath).lexically_relative("assets");

    const std::string resolvedPath = absoluteTexturePath.string();


    unsigned char* imageData = stbi_load(
        resolvedPath.c_str(),
        &width,
        &height,
        &channels,
        0
    );

    if (imageData == nullptr)
    {
        std::cerr << "Could not load texture: " << resolvedPath << '\n';
        std::cerr << "Reason: "
            << (stbi_failure_reason() != nullptr
                ? stbi_failure_reason()
                : "Unknown image loading error")
            << '\n';
        return false;
    }

    GLenum format = GL_RGB;

    if (channels == 1)
    {
        format = GL_RED;
    }
    else if (channels == 3)
    {
        format = GL_RGB;
    }
    else if (channels == 4)
    {
        format = GL_RGBA;
    }
    else
    {
        std::cerr << "Unsupported texture channel count: "
            << channels << '\n';
        stbi_image_free(imageData);
        return false;
    }

    stbi_set_flip_vertically_on_load(true);

    // Generate a real OpenGL texture ID before binding it.
    glGenTextures(1, &textureId);

    if (textureId == 0)
    {
        std::cerr << "Failed to generate OpenGL texture ID.\n";
        stbi_image_free(imageData);
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<GLint>(format),
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        imageData
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    stbi_image_free(imageData);

    glBindTexture(GL_TEXTURE_2D, 0);

    loadedFromFile = true;

    return true;
}



void Texture::createCheckerTexture()
{
    constexpr int textureWidth = 64;
    constexpr int textureHeight = 64;
    constexpr int squareSize = 8;

    unsigned char pixels[
        textureWidth * textureHeight * 4
    ]{};

        for (int y = 0; y < textureHeight; ++y)
        {
            for (int x = 0; x < textureWidth; ++x)
            {
                const bool alternate =
                    ((x / squareSize) + (y / squareSize)) % 2 == 0;

                const unsigned char value =
                    alternate ? 220 : 90;

                const int index =
                    (y * textureWidth + x) * 4;

                pixels[index + 0] = value;
                pixels[index + 1] = value;
                pixels[index + 2] = value;
                pixels[index + 3] = 255;
            }
        }

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            textureWidth,
            textureHeight,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels
        );

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_REPEAT
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_REPEAT
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR_MIPMAP_LINEAR
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR
        );

        glBindTexture(GL_TEXTURE_2D, 0);

        // The checker texture is a silent fallback; the original load failure
        // is already reported by loadFromFile().
}
