// FileDialog.hpp
#pragma once
#include <string>
#include <optional>

class FileDialog
{
public:
    // filter example: "Images\0*.png\0Models\0*.obj;*.fbx\0"
    static std::optional<std::string> openFile(const char* title, const char* filter);
};