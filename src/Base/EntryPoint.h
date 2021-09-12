#pragma once

#include <Application.h>

extern Application* CreateApplication();

int main()
{
	Application* app = CreateApplication();
	app->Run();
	delete app;
}