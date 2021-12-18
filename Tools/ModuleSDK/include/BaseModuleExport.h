#pragma once

#include "ModuleAPI.h"

extern "C" MODULE_API char* GetModuleName();

extern "C" MODULE_API char* GetModuleVersion();

extern "C" MODULE_API bool VerifyUpdate(char*);