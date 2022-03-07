#pragma once

#include <string>
#include <Mesh.h>

class Model;

void ExportModelAssimp(Model* model, std::string , std::string path, std::string extension = "");