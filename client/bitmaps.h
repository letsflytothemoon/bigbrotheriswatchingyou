#pragma once
#include <Windows.h>

std::vector<unsigned char> CreateBMPFile(HDC hDC, HBITMAP hBMP);
std::vector<unsigned char> TakeScreenShot(int);