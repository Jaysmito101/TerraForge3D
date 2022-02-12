#pragma once
#include "EntryPoint.h"
#include <iostream>
#include <string>

#ifdef TERR3D_WIN32

#include <windows.h>

static void AllocateConsole() {
	
}
static std::string ParseArgs(PWSTR pCmdLine) {
	LPWSTR* szArgList;
	int argCount;

	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
	if (szArgList == NULL)
	{
		MessageBox(NULL, L"Unable to load file from command line argument! Opening blank project.", L"Error", MB_OK);
		return std::string("");
	}

	if (argCount == 2) {
		std::wstring ws(szArgList[1]);
		return std::string(ws.begin(), ws.end());
	}
	return std::string("");

	LocalFree(szArgList);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	Application* app = CreateApplication();
	app->OnPreload();
	app->Init();
	{
		AllocateConsole();
		app->OnStart(ParseArgs(pCmdLine));
	}
	app->Run();
	delete app;
}

#else

int main(int argc, char** argv) 
{
	Application* app = CreateApplication();
	app->OnPreload();
	app->Init();
	{
		std::string args = "";
		if (argc == 2)
			argc = std::string(argv[1]);
		app->OnStart(args);
	}
	app->Run();
	delete app;
}

#endif