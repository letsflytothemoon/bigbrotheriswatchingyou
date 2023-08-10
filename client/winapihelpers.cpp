#include <sstream>
#include "winapihelpers.h"

namespace winapi
{
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
}