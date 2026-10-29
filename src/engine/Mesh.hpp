
#pragma once

#include <glad/gl.h>

#include <cstddef>

class Mesh
{
public:
    Mesh(
        const float* vertices,
        std::size_t vertexFloatCount,
        const unsigned int* indices,
        std::size_t indexCount
    );

    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&&) = delete;
    Mesh& operator=(Mesh&&) = delete;

    bool isValid() const;

    void draw() const;

private:
    GLuint vertexArray;
    GLuint vertexBuffer;
    GLuint indexBuffer;

    GLsizei indexCount;
};
