/******************************************************************
 * This file is part of TerraForge3D.
 * 
 * Maintainers:
 * - Jaysmito Mukherjee
 * 
 * Copyright: Jaysmito Mukherjee
 * MIT License
 * 
 ******************************************************************/

#include "EntryPoint.hpp"

// STD includes
#include <iostream>
#include <string>
#include <vector>

#ifdef TF3D_WINDOWS

// Includes
#include <shellapi.h>

// STD Include sprcifically for Windows
#include <locale>
#include <codecvt>

/**
 * Allocates a console for the application.
 *
 * Since this is a Windowed Application, we need to allocate a console for it manually.
 */
static void AllocateConsole()
{
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    ShowWindow (GetConsoleWindow(), SW_SHOW);
}

/**
 * Parse the Aruments from the PWSRT to a vector of strings.
 * 
 */
static std::vector<std::string> ParseArguments()
{
    std::vector<std::string> args;
    LPWSTR *szArgList;
    int argCount;
    szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
    if (szArgList == nullptr)
    {
        return args;
    }
    for (int i = 0; i < argCount; i++)
    {
        std::wstring ws(szArgList[i]);
        using convertType = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convertType, wchar_t> converter;
        std::string s(converter.to_bytes(ws));
        args.push_back(s);
    }
    LocalFree(szArgList);
    return args;
}

/**
 * The main entry point for the application.
 * 
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    AllocateConsole();
    std::vector<std::string> args = ParseArguments();
    TerraForge3D::Application* mainApplication = CreateApplication();
    mainApplication->Run();
    delete mainApplication;
}

#else

/**
 * Parse the arguments from char** to a vector of strings.
 * 
 */
static std::vector<std::string> ParseArguments(int argc, char* argv[])
{
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++)
    {
        std::string s(argv[i]);
        args.push_back(s);
    }
    return args;
}

/**
 * The main entry point for the application.
 * 
 */
int main(int argc, char* argv[], char* envp[])
{
    std::vector<std::string> args = ParseArguments(argc, argv);
    TerraForge3D::Application* mainApplication = CreateApplication();
    mainApplication->Run();
    delete mainApplication;
}

#endif