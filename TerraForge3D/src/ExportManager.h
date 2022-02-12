#pragma once

#include <string>
#include <Mesh.h>

class Model;

bool ExportOBJ(Mesh* mesh, std::string filename);

bool ExportHeightmapPNG(Mesh* mesh, std::string filename);

bool ExportHeightmapJPG(Mesh* mesh, std::string filename);

void ExportModelAssimp(Model* model, std::string , std::string path, std::string extension = "");