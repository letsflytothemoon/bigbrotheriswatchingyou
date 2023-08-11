#pragma once
#include <windows.h>
#include <string>

namespace winapi
{
    int Install();

    int Uninstall();

    std::string ModuleFileName();

    std::string CurrentDirectory();

    std::string ModuleFileDirectory();

    void MsgBox(std::string title, std::string text);

    HWND CreateInvisibleWindow(HINSTANCE hInstance, std::string className, WNDPROC wndproc);
}