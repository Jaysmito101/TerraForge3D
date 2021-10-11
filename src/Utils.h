#pragma once

#include <Windows.h>
#include <string>

#define MAX(x, y) (x > y ? x : y)

std::string ShowSaveFileDialog(std::string ext = ".terr3d", HWND owner = NULL);

std::string openfilename(HWND owner = NULL);

std::string ShowOpenFileDialog(LPWSTR ext = (wchar_t*)L"*.glsl\0*.*\0", HWND owner = NULL);

std::string ReadShaderSourceFile(std::string path, bool* result);

std::string GetExecutableDir();

std::string FetchURL(std::string baseURL, std::string path);

bool FileExists(std::string path, bool writeAccess = false);

void DownloadFile(std::string baseURL, std::string urlPath, std::string path, int size = -1);

void SaveToFile(std::string filename, std::string content = "");

void Log(const char* log);

void Log(std::string log);