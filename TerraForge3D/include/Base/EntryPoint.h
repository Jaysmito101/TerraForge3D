#pragma once
#include "Base/Application.h"

extern Application *CreateApplication();


#ifdef TERR3D_WIN32
#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);

#else

int main(int argc, char **argv);

#endif
