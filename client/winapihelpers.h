#pragma once
#include <windows.h>
#include <string>

namespace winapi
{
    std::string ModuleFileName();

    std::string CurrentDirectory();

    std::string ModuleFileDirectory();

    void MsgBox(std::string title, std::string text);
}