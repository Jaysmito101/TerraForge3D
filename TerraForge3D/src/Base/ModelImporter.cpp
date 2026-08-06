#include "ModelImporter.h"

#include <string>

#include "Utils/Utils.h"

namespace tf3d::base
{

    Model *LoadModel(std::string path)
    {
        // TODO: Implement this
        Model *model = new Model("Invalid Model");
        model->mesh->GeneratePlane(256, 1.0f);
        model->mesh->RecalculateNormals();
        model->SetupMeshOnGPU();
        return model;
    }

} // namespace tf3d::base
