#include "Generators/CPUNodeEditor/Nodes/NoiseValueCubicNode.h"
#include "FastNoiseLite/FastNoiseLite.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

static const char *fractalTypes[] = { "None", "FBm", "Ridged", "PingPong" };


#define SHOW_INT_SETTINGS_PIN(pinNumber, variableName, textLabel, sliderSpeed)  \
{ \
	inputPins[pinNumber]->Render(); ImGui::Text(textLabel); \
	if (!inputPins[pinNumber]->IsLinked()) \
	{ \
		ImGui::Dummy(ImVec2(30, 10)); \
		ImGui::SameLine(); \
		ImGui::PushItemWidth(100); \
		UPDATE_HAS_CHHANGED(ImGui::DragInt(MAKE_IMGUI_ID(inputPins[pinNumber]->id), &variableName, sliderSpeed)); \
		ImGui::PopItemWidth(); \
	} \
	else ImGui::NewLine(); \
} 

#define SHOW_FLOAT_SETTINGS_PIN(pinNumber, variableName, textLabel, sliderSpeed)  \
{ \
	inputPins[pinNumber]->Render(); ImGui::Text(textLabel); \
	if (!inputPins[pinNumber]->IsLinked()) \
	{ \
		ImGui::Dummy(ImVec2(30, 10)); \
		ImGui::SameLine(); \
		ImGui::PushItemWidth(100); \
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(MAKE_IMGUI_ID(inputPins[pinNumber]->id), &variableName, sliderSpeed)); \
		ImGui::PopItemWidth(); \
	} \
	else ImGui::NewLine(); \
}


NodeOutput NoiseValueCubicNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	if (!inputPins[0]->IsLinked()) noiseGen->SetSeed(seed);
	else noiseGen->SetSeed((int)inputPins[0]->other->Evaluate(input).value);

	if (!inputPins[2]->IsLinked()) noiseGen->SetFrequency(frequency);
	else noiseGen->SetFrequency(inputPins[2]->other->Evaluate(input).value);

	if (fractalType == 0) noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_None);
	else if (fractalType == 1) noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_FBm);
	else if (fractalType == 2) noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_Ridged);
	else if (fractalType == 3) noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_PingPong);
	else noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_None);

	if (!inputPins[1]->IsLinked()) noiseGen->SetFractalOctaves(octaves);
	else noiseGen->SetFrequency(inputPins[1]->other->Evaluate(input).value);

	if (!inputPins[3]->IsLinked()) noiseGen->SetFractalLacunarity(lacunarity);
	else noiseGen->SetFractalLacunarity(inputPins[3]->other->Evaluate(input).value);

	if (!inputPins[4]->IsLinked()) noiseGen->SetFractalGain(gain);
	else noiseGen->SetFractalGain(inputPins[4]->other->Evaluate(input).value);

	if (!inputPins[5]->IsLinked()) noiseGen->SetFractalWeightedStrength(weightedStrength);
	else noiseGen->SetFractalWeightedStrength(inputPins[5]->other->Evaluate(input).value);

	if (!inputPins[6]->IsLinked()) noiseGen->SetFractalPingPongStrength(pingPongStrength);
	else noiseGen->SetFractalPingPongStrength(inputPins[6]->other->Evaluate(input).value);

	float st = strength;
	if (inputPins[7]->IsLinked()) st = inputPins[7]->other->Evaluate(input).value;

	float yV = yVal;
	if (inputPins[8]->IsLinked()) yV = inputPins[8]->other->Evaluate(input).value;

	return NodeOutput({ noiseGen->GetNoise(input.x, yV, input.z) * st });
}

void NoiseValueCubicNode::Load(nlohmann::json data)
{
	frequency = data["frequency"];
	seed = data["seed"];
	lacunarity = data["lacunarity"];
	weightedStrength = data["weightedStrength"];
	octaves = data["octaves"];
	pingPongStrength = data["pingPongStrength"];
	yVal = data["yVal"];
	gain = data["gain"];
	strength = data["strength"];
	fractalType = data["fractalType"];
}

nlohmann::json NoiseValueCubicNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::NoiseValueCubic;
	data["frequency"] = frequency;
	data["seed"] = seed;
	data["lacunarity"] = lacunarity;
	data["weightedStrength"] = weightedStrength;
	data["octaves"] = octaves;
	data["yVal"] = yVal;
	data["pingPongStrength"] = pingPongStrength;
	data["gain"] = gain;
	data["strength"] = strength;
	data["fractalType"] = fractalType;
	return data;
}

void NoiseValueCubicNode::OnRender()
{
	DrawHeader("ValueCubic Noise");
	ImGui::Dummy(ImVec2(150, 10));

	ImGui::SameLine(); ImGui::Text("Out"); outputPins[0]->Render();

	SHOW_INT_SETTINGS_PIN(0, seed, "Seed", 1);
	SHOW_INT_SETTINGS_PIN(1, octaves, "Octaves", 0.1f);
	SHOW_FLOAT_SETTINGS_PIN(2, frequency, "Frequency", 0.001f);
	SHOW_FLOAT_SETTINGS_PIN(3, lacunarity, "Lacunarity", 0.01f);
	SHOW_FLOAT_SETTINGS_PIN(4, gain, "Gain", 0.01f);
	SHOW_FLOAT_SETTINGS_PIN(5, weightedStrength, "Weighted Strength", 0.01f);
	SHOW_FLOAT_SETTINGS_PIN(6, pingPongStrength, "Ping Pong Strength", 0.01f);
	SHOW_FLOAT_SETTINGS_PIN(7, strength, "Strength", 0.01f);
	SHOW_FLOAT_SETTINGS_PIN(8, yVal, "Layer Offset", 0.01f);

	ImGui::NewLine(); ImGui::Text("Current Fractal Type : "); ImGui::SameLine(); ImGui::Text(fractalTypes[fractalType]);
	if (ImGui::Button(MAKE_IMGUI_LABEL(id, "Change Fractal Type"))) { fractalType = (fractalType + 1) % 4; hasChanged = true; }
}

NoiseValueCubicNode::NoiseValueCubicNode()
{
	headerColor = ImColor(NOISE_NODE_COLOR);
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	seed = 42; frequency = 1.01f; fractalType = 0; octaves = 3;
	lacunarity = 2.0f; yVal = 0.0f; gain = 0.5f; weightedStrength = 0.0f; // should be within 0 to 1
	pingPongStrength = 2.0f; strength = 1.0f;
	noiseGen = new FastNoiseLite();
	noiseGen->SetNoiseType(FastNoiseLite::NoiseType::NoiseType_ValueCubic);
}

NoiseValueCubicNode::~NoiseValueCubicNode()
{
	delete noiseGen;
}
