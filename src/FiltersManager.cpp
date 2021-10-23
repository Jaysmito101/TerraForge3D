#include <FiltersManager.h>
#include <Filters/SnowballErosionFilters.h>
#include <imgui.h>

static bool* autoUpdate;
static Model* mainModel;

void SetupFiltersManager(bool* aU, Model* model){
    autoUpdate = aU;
    mainModel = model;
}

void ShowFiltersMamager(bool* pOpen){
    ImGui::Begin("Filters Manager", pOpen);
    if(*autoUpdate){
        ImGui::Text("Cannot use filters while auto update is on!");
    }else{
        bool state = ImGui::CollapsingHeader("##SnowBallErrosionFilterSettings");
	    ImGui::SameLine();
	    ImGui::Text("Snowball Erosion");
	    if (state) {
            RenderSnowballErosionFilterPropUI();
            if(ImGui::Button("Apply")){
                ApplySnowballErosion(mainModel);
            }
	    }
    }
    ImGui::End();
}