#include "ObjLoader.hpp"

#include <glm/glm.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef SIMPLERENDERER_ASSET_DIR
#define SIMPLERENDERER_ASSET_DIR "assets"
#endif

namespace
{
    struct ObjVertexReference
    {
        int position = 0;
        int texCoord = 0;
        int normal = 0;
    };

    bool parseReference(const std::string& token, ObjVertexReference& result)
    {
        std::stringstream stream(token);
        std::string part;
        int component = 0;

        while (std::getline(stream, part, '/'))
        {
            if (!part.empty())
            {
                try
                {
                    const int value = std::stoi(part);
                    if (component == 0) result.position = value;
                    else if (component == 1) result.texCoord = value;
                    else if (component == 2) result.normal = value;
                }
                catch (...)
                {
                    return false;
                }
            }
            ++component;
        }

        return result.position != 0;
    }

    bool resolveIndex(int objIndex, std::size_t count, std::size_t& resolved)
    {
        if (objIndex > 0)
        {
            const auto index = static_cast<std::size_t>(objIndex - 1);
            if (index >= count) return false;
            resolved = index;
            return true;
        }

        if (objIndex < 0)
        {
            const auto distance = static_cast<std::size_t>(-objIndex);
            if (distance == 0 || distance > count) return false;
            resolved = count - distance;
            return true;
        }

        return false;
    }
}

bool ObjLoader::load(
    const std::string& filePath,
    ObjMeshData& output,
    std::string& errorMessage
)
{
    output.vertices.clear();
    output.indices.clear();
    errorMessage.clear();

    const std::filesystem::path inputPath(filePath);
    const std::filesystem::path resolvedPath = inputPath.is_absolute()
        ? inputPath
        : std::filesystem::path(SIMPLERENDERER_ASSET_DIR) /
            inputPath.lexically_relative("assets");

    std::ifstream file(resolvedPath);
    if (!file)
    {
        errorMessage = "Could not open OBJ file: " + resolvedPath.string();
        return false;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;

    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;
        std::istringstream lineStream(line);
        std::string prefix;
        lineStream >> prefix;

        if (prefix.empty() || prefix[0] == '#') continue;

        if (prefix == "v")
        {
            glm::vec3 position;
            if (!(lineStream >> position.x >> position.y >> position.z))
            {
                errorMessage = "Invalid vertex at line " + std::to_string(lineNumber);
                return false;
            }
            positions.push_back(position);
        }
        else if (prefix == "vt")
        {
            glm::vec2 uv;
            if (!(lineStream >> uv.x >> uv.y))
            {
                errorMessage = "Invalid texture coordinate at line " + std::to_string(lineNumber);
                return false;
            }
            texCoords.push_back(uv);
        }
        else if (prefix == "vn")
        {
            glm::vec3 normal;
            if (!(lineStream >> normal.x >> normal.y >> normal.z))
            {
                errorMessage = "Invalid normal at line " + std::to_string(lineNumber);
                return false;
            }
            normals.push_back(glm::normalize(normal));
        }
        else if (prefix == "f")
        {
            std::vector<ObjVertexReference> face;
            std::string token;
            while (lineStream >> token)
            {
                ObjVertexReference reference;
                if (!parseReference(token, reference))
                {
                    errorMessage = "Invalid face reference at line " + std::to_string(lineNumber);
                    return false;
                }
                face.push_back(reference);
            }

            if (face.size() < 3)
            {
                errorMessage = "Face has fewer than three vertices at line " + std::to_string(lineNumber);
                return false;
            }

            // Fan-triangulate polygons; duplicate triangle vertices so missing
            // normals can be generated per face and OBJ UV seams remain correct.
            for (std::size_t corner = 1; corner + 1 < face.size(); ++corner)
            {
                const ObjVertexReference triangle[3] = { face[0], face[corner], face[corner + 1] };
                glm::vec3 trianglePositions[3];
                glm::vec2 triangleUvs[3]{};
                glm::vec3 triangleNormals[3]{};
                bool hasAllNormals = true;

                for (int i = 0; i < 3; ++i)
                {
                    std::size_t index = 0;
                    if (!resolveIndex(triangle[i].position, positions.size(), index))
                    {
                        errorMessage = "Position index out of range at line " + std::to_string(lineNumber);
                        return false;
                    }
                    trianglePositions[i] = positions[index];

                    if (triangle[i].texCoord != 0)
                    {
                        if (!resolveIndex(triangle[i].texCoord, texCoords.size(), index))
                        {
                            errorMessage = "Texture coordinate index out of range at line " + std::to_string(lineNumber);
                            return false;
                        }
                        triangleUvs[i] = texCoords[index];
                    }

                    if (triangle[i].normal != 0)
                    {
                        if (!resolveIndex(triangle[i].normal, normals.size(), index))
                        {
                            errorMessage = "Normal index out of range at line " + std::to_string(lineNumber);
                            return false;
                        }
                        triangleNormals[i] = normals[index];
                    }
                    else
                    {
                        hasAllNormals = false;
                    }
                }

                if (!hasAllNormals)
                {
                    const glm::vec3 edgeA = trianglePositions[1] - trianglePositions[0];
                    const glm::vec3 edgeB = trianglePositions[2] - trianglePositions[0];
                    const glm::vec3 crossProduct = glm::cross(edgeA, edgeB);
                    const glm::vec3 generatedNormal = glm::length(crossProduct) > 0.00001f
                        ? glm::normalize(crossProduct)
                        : glm::vec3(0.0f, 1.0f, 0.0f);
                    for (auto& normal : triangleNormals) normal = generatedNormal;
                }

                for (int i = 0; i < 3; ++i)
                {
                    output.vertices.insert(output.vertices.end(), {
                        trianglePositions[i].x, trianglePositions[i].y, trianglePositions[i].z,
                        triangleNormals[i].x, triangleNormals[i].y, triangleNormals[i].z,
                        triangleUvs[i].x, triangleUvs[i].y
                    });
                    output.indices.push_back(static_cast<unsigned int>(output.indices.size()));
                }
            }
        }
    }

    if (output.vertices.empty() || output.indices.empty())
    {
        errorMessage = "OBJ file contains no renderable faces: " + resolvedPath.string();
        return false;
    }

    return true;
}
