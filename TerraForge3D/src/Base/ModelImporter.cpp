#include "ModelImporter.h"

#include <string>

#include <Utils.h>



Model *LoadModel(std::string path)
{
	// TODO: Implement this
	Model *model = new Model("Invalid Model");
	model->mesh->GeneratePlane(256, 1.0f);
	model->mesh->RecalculateNormals();
	model->SetupMeshOnGPU();
	return model;
}
