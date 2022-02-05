#include "ClearMeshGenerator.h"
#include "Utils.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"
ClearMeshGenerator::ClearMeshGenerator(ApplicationState* as, ComputeKernel* kernels)
{
    bool tmp = false;
    appState = as;
}

void ClearMeshGenerator::Generate(ComputeKernel* kernels)
{
    START_PROFILER();

    if (appState->mode == ApplicationMode::TERRAIN)
    {
        kernels->SetKernelArg("clear_mesh_terrain", 0, "mesh");

        kernels->ExecuteKernel("clear_mesh_terrain", cl::NDRange(1), cl::NDRange(appState->models.coreTerrain->mesh->vertexCount));
    }
    else if (appState->mode == ApplicationMode::CUSTOM_BASE)
    {        

        kernels->SetKernelArg("clear_mesh_custom_base", 0, "mesh");
        kernels->SetKernelArg("clear_mesh_custom_base", 1, "mesh_copy");

        kernels->ExecuteKernel("clear_mesh_custom_base", cl::NDRange(1), cl::NDRange(appState->models.customBase->mesh->vertexCount));
    }

    END_PROFILER(time);
}

nlohmann::json ClearMeshGenerator::Save()
{
    return nlohmann::json();
}

void ClearMeshGenerator::Load(nlohmann::json data)
{
}

void ClearMeshGenerator::ShowSettings()
{
    ImGui::Text("Time : %lf ms", time);
}
