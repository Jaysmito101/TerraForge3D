#pragma once

#include <string>
#include <Mesh.h>

bool ExportOBJ(Mesh mesh, std::string filename);

bool ExportHeightmapPNG(Mesh mesh, std::string filename);