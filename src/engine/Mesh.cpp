
#include "Mesh.hpp"

Mesh::Mesh(
    const float* vertices,
    std::size_t vertexFloatCount,
    const unsigned int* indices,
    std::size_t indexCount
)
    : vertexArray(0),
    vertexBuffer(0),
    indexBuffer(0),
    indexCount(static_cast<GLsizei>(indexCount))
{
    if (vertices == nullptr ||
        indices == nullptr ||
        vertexFloatCount == 0 ||
        indexCount == 0)
    {
        return;
    }

    glGenVertexArrays(1, &vertexArray);
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(1, &indexBuffer);

    glBindVertexArray(vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            vertexFloatCount * sizeof(float)
            ),
        vertices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            indexCount * sizeof(unsigned int)
            ),
        indices,
        GL_STATIC_DRAW
    );

    // Vertex layout:
    // Position: 3 floats
    // Normal:   3 floats
    // UV:       2 floats
    constexpr GLsizei vertexStride =
        8 * static_cast<GLsizei>(sizeof(float));

    // Attribute 0: position.
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        vertexStride,
        nullptr
    );

    glEnableVertexAttribArray(0);

    // Attribute 1: surface normal.
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        vertexStride,
        reinterpret_cast<void*>(
            3 * sizeof(float)
            )
    );

    glEnableVertexAttribArray(1);

    // Attribute 2: texture coordinates.
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        vertexStride,
        reinterpret_cast<void*>(
            6 * sizeof(float)
            )
    );

    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

Mesh::~Mesh()
{
    if (indexBuffer != 0)
    {
        glDeleteBuffers(1, &indexBuffer);
    }

    if (vertexBuffer != 0)
    {
        glDeleteBuffers(1, &vertexBuffer);
    }

    if (vertexArray != 0)
    {
        glDeleteVertexArrays(1, &vertexArray);
    }
}

bool Mesh::isValid() const
{
    return vertexArray != 0
        && vertexBuffer != 0
        && indexBuffer != 0
        && indexCount > 0;
}

void Mesh::draw() const
{
    if (!isValid())
    {
        return;
    }

    glBindVertexArray(vertexArray);

    glDrawElements(
        GL_TRIANGLES,
        indexCount,
        GL_UNSIGNED_INT,
        nullptr
    );

    glBindVertexArray(0);
}
