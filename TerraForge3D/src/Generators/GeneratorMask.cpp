#include "Generators/GeneratorMask.h"
#include "Generators/MaskLayerHelper.h"
#include "Data/ApplicationState.h"

#include "imgui/imgui.h"
#include "OpenCL/OpenCLContext.h"


GeneratorMaskManager::GeneratorMaskManager(OpenCLContext* kernel, std::string id, ApplicationState *as)
{
	uid = id;
	appState = as;

	if(kernel)
	{
		kernel->CreateBuffer("mask_items_count", CL_MEM_READ_WRITE, sizeof(int));
		kernel->CreateBuffer("mask_items", CL_MEM_READ_WRITE, sizeof(GeneratorMask) * MAX_GENERATOR_MASKS);
	}
}


GeneratorMaskManager::~GeneratorMaskManager()
{
}

nlohmann::json GeneratorMaskManager::SaveGeneratorMask(GeneratorMask mask)
{
	nlohmann::json data;
	data["type"] = mask.type;
	data["posX"] = mask.pos[0];
	data["posY"] = mask.pos[1];
	data["posZ"] = mask.pos[2];
	data["d1.0"] = mask.d1[0];
	data["d1.1"] = mask.d1[1];
	data["d1.2"] = mask.d1[2];
	data["d1.3"] = mask.d1[3];
	data["d2.0"] = mask.d2[0];
	data["d2.1"] = mask.d2[1];
	data["d2.2"] = mask.d2[2];
	data["d2.3"] = mask.d2[3];
	data["d3.0"] = mask.d3[0];
	data["d3.1"] = mask.d3[1];
	data["d3.2"] = mask.d3[2];
	data["d3.3"] = mask.d3[3];
	data["d4.0"] = mask.d4[0];
	data["d4.1"] = mask.d4[1];
	data["d4.2"] = mask.d4[2];
	data["d4.3"] = mask.d4[3];
	return data;
}

GeneratorMask GeneratorMaskManager::LoadGeneratorMask(nlohmann::json data)
{
	GeneratorMask mask;
	mask.type = data["type"]  ;
	mask.pos[0] = data["posX"];
	mask.pos[1] = data["posY"];
	mask.pos[2] = data["posZ"];
	mask.d1[0] = data["d1.0"];
	mask.d1[1] = data["d1.1"];
	mask.d1[2] = data["d1.2"];
	mask.d1[3] = data["d1.3"];
	mask.d2[0] = data["d2.0"];
	mask.d2[1] = data["d2.1"];
	mask.d2[2] = data["d2.2"];
	mask.d2[3] = data["d2.3"];
	mask.d3[0] = data["d3.0"];
	mask.d3[1] = data["d3.1"];
	mask.d3[2] = data["d3.2"];
	mask.d3[3] = data["d3.3"];
	mask.d4[0] = data["d4.0"];
	mask.d4[1] = data["d4.1"];
	mask.d4[2] = data["d4.2"];
	mask.d4[3] = data["d4.3"];
	return mask;
}

nlohmann::json GeneratorMaskManager::Save()
{
	nlohmann::json data;
	data["uid"] = uid;
	data["enabled"] = enabled;
	data["gmcount"] = gmcount;
	data["count"] = masks.size();
	data["type"] = type;
	nlohmann::json msks;

	for(int i=0; i<masks.size(); i++)
	{
		msks.push_back(SaveGeneratorMask(masks[i]));
	}

	data["masks"] = msks;
	return data;
}

void GeneratorMaskManager::Load(nlohmann::json data)
{
	uid = data["uid"];
	enabled = data["enabled"];
	gmcount = data["gmcount"];
	type = data["type"];
	masks.clear();

	for(int i=0; i<data["count"]; i++) masks.push_back(LoadGeneratorMask(data["masks"][i]));
}

float GeneratorMaskManager::EvaluateAt(float x, float y, float z, float value) const
{
	float m = 0.0f;

	for(int i=0; i<masks.size(); i++)
	{
		if(masks[i].type == MASK_LAYER_HILL) m += EvaluateHillMask(&masks[i], x, y, z);
		else if(masks[i].type == MASK_LAYER_CRATOR) m += EvaluateCratorMask(&masks[i], x, y, z);
		else if(masks[i].type == MASK_LAYER_CLIFF) m += EvaluateCliffMask(&masks[i], x, y, z);
	}

	switch(type)
	{
		case GeneratorMask_Additive:
		{
			value = value + m;
			break;
		}

		case GeneratorMask_AverageAdditive:
		{
			m = m / (float)masks.size();
			value = value + m;
			break;
		}

		case GeneratorMask_Multiplicative:
		{
			value = value * m;
			break;
		}

		case GeneratorMask_AverageMultiplicative:
		{
			m = m / (float)masks.size();
			value = value * m;
			break;
		}

		case GeneratorMask_AddMul:
		{
			value = value * m + m;
			break;
		}


		default:
			break;
	};

	return value;
}

bool GeneratorMaskManager::ShowSettings()
{
	bool stateChanged = false;
	stateChanged |= ImGui::Checkbox(("Enabled##GMSK" + uid).c_str(), &enabled);

	if (ImGui::BeginCombo("Masking Mode##GMSK", generator_mask_type_names[type]))
	{
		for (int n = 0; n < IM_ARRAYSIZE(generator_mask_type_names); n++)
		{
			bool is_selected = (type == n);
			if (ImGui::Selectable(generator_mask_type_names[n], is_selected))
			{
				type = (GeneratorMaskType)n;
				stateChanged |= true;
			}
			if (is_selected) ImGui::SetItemDefaultFocus();			
		}
		ImGui::EndCombo();
	}

	for(int i=0; i<masks.size(); i++)
	{
		if(ImGui::CollapsingHeader(("Custom Base Mask Layer " + std::to_string(i) + "##GMSK" + uid).c_str()))
		{
			if(masks[i].type == MASK_LAYER_HILL) stateChanged |= ShowHillMaskSettingS(&masks[i], uid + std::to_string(i));
			else if(masks[i].type == MASK_LAYER_CRATOR) stateChanged |= ShowCratorMaskSettingS(&masks[i], uid + std::to_string(i));
			else if(masks[i].type == MASK_LAYER_CLIFF) stateChanged |= ShowCliffMaskSettingS(&masks[i], uid + std::to_string(i));
			if(ImGui::Button(("Delete##GMSK" + std::to_string(i) + uid).c_str()))
			{
				appState->workManager->WaitForFinish();
				masks.erase(masks.begin() + i);
				break;
			}
		}
		ImGui::Separator();
	}

	if(ImGui::Button(("Add##GMSK" + uid).c_str()))
	{
		ImGui::OpenPopup(("AddMaskLayer##GMSK" + uid).c_str());
	}

	if(ImGui::BeginPopup(("AddMaskLayer##GMSK" + uid).c_str()))
	{
		if(ImGui::Button(("Hill##GMSK" + uid).c_str()))
		{
			masks.push_back(GeneratorMask());
			masks.back().type = MASK_LAYER_HILL;
			masks.back().d1[0] = masks.back().d1[1] = masks.back().d1[2] = 1.0f;
			gmcount++;
			ImGui::CloseCurrentPopup();
			stateChanged |= true;
		}

		if(ImGui::Button(("Crater##GMSK" + uid).c_str()))
		{
			masks.push_back(GeneratorMask());
			masks.back().type = MASK_LAYER_CRATOR;
			masks.back().d1[0] = masks.back().d3[0] = masks.back().d3[1] = 1.0f;
			gmcount++;
			ImGui::CloseCurrentPopup();
			stateChanged |= true;
		}

		if(ImGui::Button(("Cliff##GMSK" + uid).c_str()))
		{
			masks.push_back(GeneratorMask());
			masks.back().type = MASK_LAYER_CLIFF;
			masks.back().d1[2] = masks.back().d1[1] = 1.0f;
			gmcount++;
			ImGui::CloseCurrentPopup();
			stateChanged |= true;
		}

		ImGui::EndPopup();
	}
	return stateChanged;
}