#pragma once
#include <windows.h>
#include <string>

namespace SplashScreen {


	void Init(HINSTANCE hInstance);
	void Destory();
	void SetSplashMessage(std::string message);
	void HideSplashScreen();
	void ShowSplashScreen();
}