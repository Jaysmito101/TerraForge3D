#pragma once
#include <EntryPoint.h>
#include <windows.h>
#include <SplashScreen.h>



#include <iostream>
#include <string>

static void AllocateConsole() {
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
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
	{
		SplashScreen::Init(hInstance);
		app->OnPreload();
		SplashScreen::Destory();
	}
	AllocateConsole();
	app->Init();
	app->Run(ParseArgs(pCmdLine));
	delete app;
}