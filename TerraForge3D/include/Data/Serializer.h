#pragma once

#include "json/json.hpp"

#include <functional>

class ApplicationState;

class Serializer
{
public:

	Serializer(ApplicationState *state);

	Serializer(ApplicationState *state, std::function<void(std::string, bool)> errorFunc);

	~Serializer();

	nlohmann::json Serialize();

	void PackProject(std::string path);

	void LoadPackedProject(std::string path);

	void SaveFile(std::string path);

	void LoadFile(std::string path);

	ApplicationState *Deserialize(nlohmann::json data);

private:
	nlohmann::json data;
	std::function<void(std::string, bool)> onError;
	ApplicationState *appState;
};