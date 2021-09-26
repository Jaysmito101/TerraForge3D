#pragma once

#include <Windows.h>
#include <string>

std::string ShowSaveFileDialog(std::string ext = ".terr3d", HWND owner = NULL);

std::string openfilename(HWND owner = NULL);

std::string ShowOpenFileDialog(const char* ext = "*.glsl\0*.*\0", HWND owner = NULL);

void Log(const char* log);

void Log(std::string log);