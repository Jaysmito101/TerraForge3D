#include "ErosionFilter.h"
#include <imgui.h>
#include <iostream>
#include <tinyerode/TinyErode.h>

#define MAX(x, y) x>y?x:y

void ErosionFilter::Render()
{
	ImGui::DragInt("Num Iterations##erosionFilter", &iterations, 1, 0);
	ImGui::DragFloat("Actual Dimensions##erosionFilter", &dimension, 1, 0);
	ImGui::DragFloat("Strength##erosionFilter", &tfac, 1000);

}

nlohmann::json ErosionFilter::Save()
{
	return nlohmann::json();
}

void ErosionFilter::Load(nlohmann::json data)
{
}

void ErosionFilter::Apply()
{
	model->mesh->RecalculateNormals();
	TinyErode::Simulation simulation(model->mesh->res, model->mesh->res);
	std::vector<float> water(simulation.GetWidth()*simulation.GetHeight());
	auto getHeight = [this](int x, int y) -> float {
		return model->mesh->GetElevation(x, y);
	};
	auto addHeight = [this](int x, int y, float deltaHeight) {
		//std::cout << deltaHeight << "\n";
		model->mesh->AddElevation(deltaHeight*tfac, x, y);
	};
	auto getWater = [&](int x, int y) -> float {
		return water[(y * model->mesh->res) + x];
	};

	auto addWater = [&](int x, int y, float deltaWater) -> float {
		return water[(y * model->mesh->res) + x] = MAX(0.0f, water[(y * model->mesh->res) + x] + deltaWater);
	};

	auto carryCapacity = [](int x, int y) -> float {
		return 0.01;
	};

	auto deposition = [](int x, int y) -> float {
		return 0.1;
	};

	auto erosion = [](int x, int y) -> float {
		return 0.1;
	};
	auto evaporation = [](int x, int y) -> float {
		return 0.1;
	};
	simulation.SetMetersPerX(dimension / model->mesh->res);
	simulation.SetMetersPerY(dimension / model->mesh->res);
	for (int i = 0; i < iterations; i++) {
		// Determines where the water will flow.
		simulation.ComputeFlowAndTilt(getHeight, getWater);
		// Moves the water around the terrain based on the previous computed values.
		simulation.TransportWater(addWater);
		// Where the magic happens. Soil is picked up from the terrain and height
		// values are subtracted based on how much was picked up. Then the sediment
		// moves along with the water and is later deposited.
		simulation.TransportSediment(carryCapacity, deposition, erosion, addHeight);
		// Due to heat, water is gradually evaported. This will also cause soil
		// deposition since there is less water to carry soil.
		simulation.Evaporate(addWater, evaporation);
		std::cout << "Finished " << (i + 1) << " iterations.\r";
	}
	// Drops all suspended sediment back into the terrain.
	simulation.TerminateRainfall(addHeight);
	std::cout << "Finished Erosion Processing!\n";
	model->mesh->RecalculateNormals();
	model->UploadToGPU();
}
