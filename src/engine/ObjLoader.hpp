#pragma once

#include <string>
#include <vector>

struct ObjMeshData
{
    // Vertex layout: position XYZ, normal XYZ, texture UV.
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

class ObjLoader
{
public:
    static bool load(
        const std::string& filePath,
        ObjMeshData& output,
        std::string& errorMessage
    );
};
