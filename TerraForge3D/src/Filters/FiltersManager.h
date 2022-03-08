#pragma once

#include <Model.h>

#include <vector>

struct ApplicationState;
class Model;
class Filter;

class FiltersManager
{
public:
	FiltersManager(ApplicationState *appState);
	~FiltersManager();

	void ShowSettings(bool *pOpen);

public:
	ApplicationState *appState;
	std::vector<Filter *> filters;
};
