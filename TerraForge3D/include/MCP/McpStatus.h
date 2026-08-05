#pragma once

#include <nlohmann/json.hpp>

class ApplicationState;

nlohmann::json BuildMcpStatus(const ApplicationState *applicationState);
