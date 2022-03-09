#pragma once
#include "Base/EntryPoint.h"
#include "Base/SplashScreen.h"
#include "Base/Logging/Logger.h"

#include <iostream>
#include <string>

#ifdef TERR3D_WIN32
#include <shellapi.h>
#include <windows.h>

static void AllocateConsole()
{
	AllocConsole();
	freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
	freopen_s((FILE **)stdin, "CONIN$", "r", stdin);
}
static std::string ParseArgs(PWSTR pCmdLine)
{
	LPWSTR *szArgList;
	int argCount;
	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);

	if (szArgList == NULL)
	{
		MessageBox(NULL, L"Unable to load file from command line argument! Opening blank project.", L"Error", MB_OK);
		return std::string("");
	}

	if (argCount == 2)
	{
		std::wstring ws(szArgList[1]);
		return std::string(ws.begin(), ws.end());
	}

	return std::string("");
	LocalFree(szArgList);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	AllocateConsole();
	Application *app = CreateApplication();
	app->OnPreload();
	Logger logger(app->logsDir);
	app->Init();
	{
		SplashScreen::Init(hInstance);
		app->OnStart(ParseArgs(pCmdLine));
		SplashScreen::Destory();
	}
	app->Run();
	delete app;
}

#else

int main(int argc, char **argv)
{
	Application *app = CreateApplication();
	app->OnPreload();
	Logger logger(app->logsDir);
	app->Init();
	{
		std::string args = "";

		if (argc == 2)
		{
			argc = std::string(argv[1]);
		}

		app->OnStart(args);
	}
	app->Run();
	delete app;
}

#endif