
#pragma once

#include <glad/gl.h>

class Shader
{
public:
    Shader(
        const char* vertexSource,
        const char* fragmentSource
    );

    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool isValid() const;

    void use() const;

    void setMatrix4(
        const char* name,
        const float* value
    ) const;

    void setVector3(
        const char* name,
        float x,
        float y,
        float z
    ) const;

    void setInteger(
        const char* name,
        int value
    ) const;

private:
    GLuint compileShader(
        GLenum type,
        const char* source
    );

    GLuint program;
};
