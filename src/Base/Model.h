#pragma once

#include <Mesh.h>

#include <string>

class Model
{
public:
	Model(std::string name);

	Mesh mesh;
	std::string name;
}