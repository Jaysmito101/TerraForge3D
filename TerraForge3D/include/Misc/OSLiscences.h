#pragma once

#include <string>
#include <vector>

class ApplicationState;

class OSLiscences
{
public:
	OSLiscences(ApplicationState *appState);
	~OSLiscences();

	void ShowSettings(bool *pOpen);

private:
	void ShowLisc(std::string &name, std::string &content, int id);

public:
	ApplicationState *appState;
	std::vector<std::pair<std::string, std::string>> osls;
};