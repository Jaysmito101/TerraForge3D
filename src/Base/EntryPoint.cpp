#pragma once
#include <EntryPoint.h>
#include <windows.h>
#include <SplashScreen.h>


#include <iostream>

static void AllocateConsole() {
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
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
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	app->Init();
	app->Run();
	delete app;
}