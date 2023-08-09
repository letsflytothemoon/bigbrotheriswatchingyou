#include <Windows.h>
#include "monitorscount.h"

int MonitorsCount()
{
    int counter = 0;
    EnumDisplayMonitors(
        NULL,
        NULL,
        [](HMONITOR monitor, HDC hDC, LPRECT rect, LPARAM param)->BOOL __stdcall
        { (*(int*)param)++; return true; },
        (LPARAM)&counter);
    return counter;
}