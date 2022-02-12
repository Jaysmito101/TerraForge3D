#pragma once
#include <Application.h>
#include <windows.h>

extern Application* CreateApplication();


#ifdef TERR3D_WIN32

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);

#else

int main(int argc, char** argv);

#endif
