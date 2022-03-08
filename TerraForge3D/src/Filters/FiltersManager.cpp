#include "FiltersManager.h"
#include "Filter.h"
#include "Data\ApplicationState.h"

#include "ErosionFilter.h"
#include "DrawFilter.h"
#include "AdvancedErosionFilter.h"
#include "GPUErosionFilter.h"

#include <imgui.h>

FiltersManager::FiltersManager(ApplicationState *as)
{
	appState = as;
	filters.clear();
	filters.push_back(new ErosionFilter(appState));
	filters.back()->OnAttach();
	filters.push_back(new AdvancedErosionFilter(appState));
	filters.back()->OnAttach();
	filters.push_back(new DrawFilter(appState));
	filters.back()->OnAttach();
}

FiltersManager::~FiltersManager()
{
	for(Filter *filter : filters)
	{
		if(filter)
		{
			delete filter;
		}
	}

	filters.clear();
}

void FiltersManager::ShowSettings(bool *pOpen)
{
	ImGui::Begin("Filters Manager", pOpen);

	if(appState->states.autoUpdate)
	{
		ImGui::Text("Cannot use filters while auto update is on!");
		ImGui::NewLine();
		ImGui::Checkbox("Auto Update", &appState->states.autoUpdate);
	}

	else
	{
		int cop = 0;

		for (Filter *filter : filters)
		{
			ImGui::Separator();
			cop++;
			bool state = ImGui::CollapsingHeader(("##" + filter->name + "-filterID" + std::to_string(cop)).c_str());
			ImGui::SameLine();
			ImGui::Text(filter->name.c_str());

			if (state)
			{
				filter->Render();

				if (ImGui::Button("Apply"))
				{
					filter->Apply();
				}
			}

			ImGui::Separator();
		}
	}

	ImGui::End();
}