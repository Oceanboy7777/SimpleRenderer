
#include "Shader.hpp"

#include <iostream>

Shader::Shader(
    const char* vertexSource,
    const char* fragmentSource
)
    : program(0)
{
    const GLuint vertexShader =
        compileShader(GL_VERTEX_SHADER, vertexSource);

    if (vertexShader == 0)
    {
        return;
    }

    const GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return;
    }

    program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success = GL_FALSE;

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE)
    {
        char infoLog[1024]{};

        glGetProgramInfoLog(
            program,
            sizeof(infoLog),
            nullptr,
            infoLog
        );

        std::cerr << "Shader linking failed:\n"
            << infoLog << '\n';

        glDeleteProgram(program);
        program = 0;
    }
}

Shader::~Shader()
{
    if (program != 0)
    {
        glDeleteProgram(program);
    }
}

bool Shader::isValid() const
{
    return program != 0;
}

void Shader::use() const
{
    glUseProgram(program);
}

void Shader::setMatrix4(
    const char* name,
    const float* value
) const
{
    const GLint location = glGetUniformLocation(program, name);

    if (location >= 0)
    {
        glUniformMatrix4fv(
            location,
            1,
            GL_FALSE,
            value
        );
    }
}

void Shader::setVector3(
    const char* name,
    float x,
    float y,
    float z
) const
{
    const GLint location = glGetUniformLocation(program, name);

    if (location >= 0)
    {
        glUniform3f(location, x, y, z);
    }
}

void Shader::setInteger(
    const char* name,
    int value
) const
{
    const GLint location = glGetUniformLocation(program, name);

    if (location >= 0)
    {
        glUniform1i(location, value);
    }
}

GLuint Shader::compileShader(
    GLenum type,
    const char* source
)
{
    const GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE)
    {
        char infoLog[1024]{};

        glGetShaderInfoLog(
            shader,
            sizeof(infoLog),
            nullptr,
            infoLog
        );

        std::cerr << "Shader compilation failed:\n"
            << infoLog << '\n';

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
