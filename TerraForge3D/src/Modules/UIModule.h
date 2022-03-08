#pragma once

#include "Terr3DModule.h"

class UIModule : public Module
{
public:
	UIModule(std::string id, ModuleManager *manager);

	void Render();

	std::string windowName;
	bool active;
};