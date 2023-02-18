#include "NoiseLayers/NoiseLayer.h"

#include "imgui.h"
#include "FastNoiseLite.h"

#define MAKE_IMGUI_ID(x) ("##NoiseLayer" + std::to_string(index) + x).c_str()
#define MAKE_IMGUI_LABEL(x) (x + std::string("##NoiseLayer") + std::to_string(index)).c_str()
#define ADD_ST_CH(x) stateChanged = x || stateChanged;

static const char *noiseTypes[] = {"Perlin", "Cellular", "OpenSimplex2", "OpenSimplex2S", "Value", "Value Cubic"};
static const char *fractalTypes[] = { "None", "FBm", "Ridged", "PingPong" };
static const char *distFuncs[] = { "EuclideanSq", "Euclidean", "Manhattan", "Euclidean Manhattan Hybrid" };

static void SetNoiseType(FastNoiseLite *noiseGen, int type)
{
	switch (type)
	{
		case 0: noiseGen->SetNoiseType(FastNoiseLite::NoiseType::NoiseType_Perlin); break;
		case 1: noiseGen->SetNoiseType(FastNoiseLite::NoiseType::NoiseType_Cellular); break;
		case 2: noiseGen->SetNoiseType(FastNoiseLite::NoiseType::NoiseType_OpenSimplex2); break;
		case 3: noiseGen->SetNoiseType(FastNoiseLite::NoiseType::NoiseType_OpenSimplex2S); break;
		case 4: noiseGen->SetNoiseType(FastNoiseLite::NoiseType::NoiseType_Value); break;
		case 5: noiseGen->SetNoiseType(FastNoiseLite::NoiseType::NoiseType_ValueCubic); break;
		default: noiseGen->SetNoiseType(FastNoiseLite::NoiseType::NoiseType_Perlin); break;
	}
}

static void SetFractalType(FastNoiseLite *noiseGen, int type)
{
	switch (type)
	{
		case 0: noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_None); break;
		case 1: noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_FBm); break;
		case 2: noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_Ridged); break;
		case 3: noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_PingPong); break;
		default: noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_None); break;
	}
}

static void SetDistanceFunc(FastNoiseLite *noiseGen, int func)
{
	switch (func)
	{
		case 0: noiseGen->SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction::CellularDistanceFunction_EuclideanSq); break;
		case 1: noiseGen->SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction::CellularDistanceFunction_Euclidean); break;
		case 2: noiseGen->SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction::CellularDistanceFunction_Manhattan); break;
		case 3: noiseGen->SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction::CellularDistanceFunction_Hybrid); break;
		default: noiseGen->SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction::CellularDistanceFunction_EuclideanSq); break;
	}
}


NoiseLayer::NoiseLayer()
{
	enabled = false; seed = 42;
	frequency = 1.0f; fractalType = 0;
	distanceFunc = 0; octaves = 8;
	lacunarity = 2.0f; gain = 0.5f;
	weightedStrength = 0.0f; // should be within 0 to 1
	pingPongStrength = 2.0f; strength = 1.0f;
	cellularJitter = 1.0f; noiseGen = new FastNoiseLite();
	name.reserve(1000); name = "Noise Layer\0                                        ";
	offset[0] = offset[1] = offset[2] = 0.0f;
	noiseType = 0; noiseTypeStr = noiseTypes[0];
	fractalTypeStr = fractalTypes[0];
	distFuncStr = distFuncs[0];
	noiseGen->SetFrequency(frequency);
}

NoiseLayer::~NoiseLayer()
{
	delete noiseGen;
}

nlohmann::json NoiseLayer::Save()
{
	nlohmann::json data;
	data["noiseType"] = noiseType;
	data["frequency"] = frequency;
	data["seed"] = seed;
	data["lacunarity"] = lacunarity;
	data["weightedStrength"] = weightedStrength;
	data["octaves"] = octaves;
	data["pingPongStrength"] = pingPongStrength;
	data["gain"] = gain;
	data["strength"] = strength;
	data["fractalType"] = fractalType;
	data["distanceFunc"] = distanceFunc;
	data["cellularJitter"] = cellularJitter;
	data["offsetX"] = offset[0];
	data["offsetY"] = offset[1];
	data["offsetZ"] = offset[2];
	data["name"] = name;
	data["enabled"] = enabled;
	return data;
}

void NoiseLayer::Load(nlohmann::json data)
{
	frequency = data["frequency"];
	noiseGen->SetFrequency(frequency);
	seed = data["seed"];
	noiseGen->SetSeed(seed);
	lacunarity = data["lacunarity"];
	noiseGen->SetFractalLacunarity(lacunarity);
	weightedStrength = data["weightedStrength"];
	noiseGen->SetFractalWeightedStrength(weightedStrength);
	octaves = data["octaves"];
	noiseGen->SetFractalOctaves(octaves);
	pingPongStrength = data["pingPongStrength"];
	noiseGen->SetFractalPingPongStrength(pingPongStrength);
	gain = data["gain"];
	noiseGen->SetFractalGain(gain);
	strength = data["strength"];
	fractalType = data["fractalType"];
	distanceFunc = data["distanceFunc"];
	cellularJitter = data["cellularJitter"];
	name = data["name"];
	enabled = data["enabled"];
	offset[0] = data["offsetX"];
	offset[1] = data["offsetY"];
	offset[2] = data["offsetZ"];
	noiseType = data["noiseType"];
	noiseTypeStr = noiseTypes[noiseType];
	fractalTypeStr = fractalTypes[fractalType];
	distFuncStr = distFuncs[distanceFunc];
	SetNoiseType(noiseGen, noiseType);
	SetFractalType(noiseGen, fractalType);
	SetDistanceFunc(noiseGen, distanceFunc);
}

float NoiseLayer::Evaluate(const NoiseLayerInput& input) const
{
	return enabled ? noiseGen->GetNoise(input.x + offset[0], input.y + offset[1], input.z + offset[2]) * strength : 0.0f;
}

bool NoiseLayer::Render(int index)
{
	bool stateChanged = false;
	ImGui::SameLine();
	ImGui::Text(name.data());
	ImGui::InputTextWithHint(MAKE_IMGUI_ID("Name Input"), "Name", name.data(), name.capacity());
	ImGui::NewLine();
	ADD_ST_CH(ImGui::Checkbox(MAKE_IMGUI_LABEL("Enabled"), &enabled));
	ImGui::Text("Offsets:");
	ADD_ST_CH(ImGui::DragFloat3(MAKE_IMGUI_ID("Offsets"), offset, 0.1f));
	ImGui::Text("Noise Type : ");
	ImGui::SameLine();

	if (ImGui::BeginCombo(MAKE_IMGUI_ID("Noise Type Selector"), noiseTypeStr))
	{
		for (int n = 0; n < 6; n++)
		{
			bool isSelected = (noiseTypeStr == noiseTypes[n]);

			if (ImGui::Selectable(noiseTypes[n], isSelected))
			{
				noiseTypeStr = noiseTypes[n]; noiseType = n;
				SetNoiseType(noiseGen, noiseType);
				stateChanged |= true;
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::Text("Fractal Type : ");
	ImGui::SameLine();

	if (ImGui::BeginCombo(MAKE_IMGUI_ID("Fractal Type Selector"), fractalTypeStr))
	{
		for (int n = 0; n < 4; n++)
		{
			bool isSelected = (fractalTypeStr == fractalTypes[n]);

			if (ImGui::Selectable(fractalTypes[n], isSelected))
			{
				fractalTypeStr = fractalTypes[n]; fractalType = n;
				SetFractalType(noiseGen, fractalType);
				stateChanged |= true;
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}

	if (ImGui::DragInt(MAKE_IMGUI_LABEL("Seed"), &seed))
	{
		stateChanged |= true;
		noiseGen->SetSeed(seed);
	}

	if (ImGui::DragFloat(MAKE_IMGUI_LABEL("Frequency"), &frequency, 0.001f))
	{
		stateChanged |= true;
		noiseGen->SetFrequency(frequency);
	}

	stateChanged |= ImGui::DragFloat(MAKE_IMGUI_LABEL("Strength"), &strength, 0.1f);

	if (fractalType != 0)
	{
		if (ImGui::DragInt(MAKE_IMGUI_LABEL("Octaves"), &octaves, 0.05f))
		{
			stateChanged |= true;
			noiseGen->SetFractalOctaves(octaves);
		}

		if (ImGui::DragFloat(MAKE_IMGUI_LABEL("Lacunarity"), &lacunarity, 0.1f))
		{
			stateChanged |= true;
			noiseGen->SetFractalLacunarity(lacunarity);
		}

		if (ImGui::DragFloat(MAKE_IMGUI_LABEL("Gain"), &gain, 0.1f))
		{
			stateChanged |= true;
			noiseGen->SetFractalGain(gain);
		}

		if (ImGui::DragFloat(MAKE_IMGUI_LABEL("Weighted Strength"), &weightedStrength, 0.01f))
		{
			stateChanged |= true;
			noiseGen->SetFractalWeightedStrength(weightedStrength);
		}

		if (ImGui::DragFloat(MAKE_IMGUI_LABEL("Ping Pong Strength"), &pingPongStrength, 0.01f))
		{
			stateChanged |= true;
			noiseGen->SetFractalPingPongStrength(pingPongStrength);
		}
	}

	if (noiseType == 1)
	{
		if (ImGui::DragFloat(MAKE_IMGUI_LABEL("Cellur Jitter"), &cellularJitter, 0.01f))
		{
			stateChanged |= true;
			noiseGen->SetCellularJitter(cellularJitter);
		}

		ImGui::Text("Cellular Distance Function : ");
		ImGui::SameLine();

		if (ImGui::BeginCombo(MAKE_IMGUI_ID("Cellular Distance Function"), distFuncStr))
		{
			for (int n = 0; n < 4; n++)
			{
				bool isSelected = (distFuncStr == distFuncs[n]);

				if (ImGui::Selectable(distFuncs[n], isSelected))
				{
					distFuncStr = distFuncs[n]; distanceFunc = n;
					SetFractalType(noiseGen, distanceFunc);
					stateChanged |= true;
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}
	}
	return stateChanged;
}
