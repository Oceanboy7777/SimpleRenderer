// FileDialog.cpp  (Windows)
#include "FileDialog.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

std::optional<std::string> FileDialog::openFile(const char* title, const char* filter)
{
    char file[MAX_PATH] = {};
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE)
        return std::string(file);
    return std::nullopt;
}
#else
std::optional<std::string> FileDialog::openFile(const char*, const char*)
{
    return std::nullopt;
}
#endif