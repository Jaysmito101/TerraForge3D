#pragma once
#include "Base/EntryPoint.h"
#include "Base/SplashScreen.h"
#include "Base/Logging/Logger.h"

#include <iostream>
#include <string>


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
			args = std::string(argv[1]);
		}
		SplashScreen::Init();
		app->OnStart(args);
		SplashScreen::Destory();
	}
	app->Run();
	delete app;
}
