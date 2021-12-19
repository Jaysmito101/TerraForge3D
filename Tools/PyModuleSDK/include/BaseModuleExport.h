#pragma once

#include "ModuleAPI.h"

extern "C" MODULE_API void LoadModule();

extern "C" MODULE_API void UnloadModule();

extern "C" MODULE_API char* GetModuleName();

extern "C" MODULE_API char* GetModuleVersion();

extern "C" MODULE_API bool VerifyUpdate(char*);