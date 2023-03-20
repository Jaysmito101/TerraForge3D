#pragma once

// Splash screen not supported on linux
#ifdef TERR3D_WIN32

#include <windows.h>
#include <string>

namespace SplashScreen
{


void Init();
void Destory();
void SetSplashMessage(std::string message);
void HideSplashScreen();
void ShowSplashScreen();
}

#else

namespace SplashScreen { void Init(){} void Destory(){} }

#endif
