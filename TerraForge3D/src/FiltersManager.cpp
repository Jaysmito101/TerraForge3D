#include <FiltersManager.h>
#include <Filters/Filter.h>
#include <imgui.h>

// Temporary
#include <Filters/ErosionFilter.h>
#include <Filters/GPUErosionFilter.h>

static bool* autoUpdate;
static Model* mainModel;

std::vector<Filter*> filters;

void SetupFiltersManager(bool* aU, Model* model){
    autoUpdate = aU;
    mainModel = model;
    filters.push_back(new ErosionFilter(model));
    filters.back()->OnAttach();
    filters.push_back(new GPUErosionFilter(model));
    filters.back()->OnAttach();
}

void ShowFiltersMamager(bool* pOpen){
    ImGui::Begin("Filters Manager", pOpen);
    if(*autoUpdate){
        ImGui::Text("Cannot use filters while auto update is on!");
    }else{
        int cop = 0;
        for (Filter* filter : filters) {
            ImGui::Separator();
            cop++;
            bool state = ImGui::CollapsingHeader(("##" + filter->name + "-filterID" + std::to_string(cop)).c_str());
            ImGui::SameLine();
            ImGui::Text(filter->name.c_str());
            if (state) {
                filter->Render();
                if (ImGui::Button("Apply")) {
                    filter->Apply();
                }
            }
            ImGui::Separator();
        }
    }
    ImGui::End();
}