
#pragma once

#include <glad/gl.h>

#include <string>

class Texture
{
public:
    Texture(
        const std::string& filePath,
        bool useCheckerFallback = true
    );

    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;

    bool isValid() const;

    void bind(unsigned int textureUnit = 0) const;

    GLuint getId() const;

    bool wasLoadedFromFile() const;

private:
    bool loadFromFile(const std::string& filePath);
    void createCheckerTexture();

    GLuint textureId;
    bool loadedFromFile;
};
