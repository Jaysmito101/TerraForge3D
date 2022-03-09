#pragma once

#include <string>

class Model;

void ExportModelAssimp(Model *model, std::string, std::string path, std::string extension = "");