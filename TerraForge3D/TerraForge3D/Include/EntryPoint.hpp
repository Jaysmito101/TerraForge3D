/******************************************************************
 * This file is part of TerraForge3D.
 * 
 * Maintainers:
 * - Jaysmito Mukherjee
 * 
 * Copyright: Jaysmito Mukherjee
 * MIT License
 * 
 ******************************************************************/

#pragma once

#include "Base/Base.hpp"

TerraForge3D::Application* CreateApplication();

#ifdef TF3D_WINDOWS

#include "Windows.h"

// WinMain for Windows
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

#else 

// main for Linux and MacOS
int main(int argc, char* argv[], char* envp[]);

#endif
