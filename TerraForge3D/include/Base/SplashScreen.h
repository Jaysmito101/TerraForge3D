#pragma once

#ifdef _WIN32
#include <string>
#include <windows.h>
#endif

namespace tf3d::base
{

// Splash screen not supported on linux
#ifdef _WIN32

    namespace SplashScreen
    {

        void Init();
        void Destory();
        void SetSplashMessage(std::string message);
        void HideSplashScreen();
        void ShowSplashScreen();
    } // namespace SplashScreen

#else

    namespace SplashScreen
    {
        void Init();
        void Destory();
    } // namespace SplashScreen

#endif

} // namespace tf3d::base
