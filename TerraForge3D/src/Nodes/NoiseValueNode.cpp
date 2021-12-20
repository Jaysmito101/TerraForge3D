#include "NoiseValueNode.h"
#include "FastNoiseLite.h"
#include "MeshNodeEditor.h"

static const char* fractalTypes[] = { "None", "FBm", "Ridged", "PingPong" };

NodeOutput NoiseValueNode::Evaluate(NodeInputParam input, NodeEditorPin* pin)
{
	if(!inputPins[0]->IsLinked())
		noiseGen->SetSeed(seed);
	else
		noiseGen->SetSeed((int)inputPins[0]->other->Evaluate(input).value);
	
	if (!inputPins[2]->IsLinked())
		noiseGen->SetFrequency(frequency);
	else
		noiseGen->SetFrequency(inputPins[2]->other->Evaluate(input).value);

	if(fractalType == 0)
		noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_None);
	else if (fractalType == 1)
		noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_FBm);
	else if (fractalType == 2)
		noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_Ridged);
	else if (fractalType == 3)
		noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_PingPong);
	else
		noiseGen->SetFractalType(FastNoiseLite::FractalType::FractalType_None);


	if (!inputPins[1]->IsLinked())
		noiseGen->SetFractalOctaves(octaves);
	else
		noiseGen->SetFrequency(inputPins[1]->other->Evaluate(input).value);

	if (!inputPins[3]->IsLinked())
		noiseGen->SetFractalLacunarity(lacunarity);
	else
		noiseGen->SetFractalLacunarity(inputPins[3]->other->Evaluate(input).value);

	if (!inputPins[4]->IsLinked())
		noiseGen->SetFractalGain(gain);
	else
		noiseGen->SetFractalGain(inputPins[4]->other->Evaluate(input).value);
	
	if (!inputPins[5]->IsLinked())
		noiseGen->SetFractalWeightedStrength(weightedStrength);
	else
		noiseGen->SetFractalWeightedStrength(inputPins[5]->other->Evaluate(input).value);
	
	if (!inputPins[6]->IsLinked())
		noiseGen->SetFractalPingPongStrength(pingPongStrength);
	else
		noiseGen->SetFractalPingPongStrength(inputPins[6]->other->Evaluate(input).value);
	
	float st = strength;
	if (inputPins[7]->IsLinked())
		st = inputPins[7]->other->Evaluate(input).value;

	return NodeOutput({ noiseGen->GetNoise(input.x, input.y, input.z) * st });
}

void NoiseValueNode::Load(nlohmann::json data)
{
	frequency = data["frequency"];
	seed = data["seed"];
	lacunarity = data["lacunarity"];
	weightedStrength = data["weightedStrength"]; 
	octaves = data["octaves"];
	pingPongStrength = data["pingPongStrength"];
	gain = data["gain"];
	strength = data["strength"];
	fractalType = data["fractalType"];
}

nlohmann::json NoiseValueNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::NoiseValue;
	data["frequency"] = frequency;
	data["seed"] = seed;
	data["lacunarity"] = lacunarity;
	data["weightedStrength"] = weightedStrength;
	data["octaves"] = octaves;
	data["pingPongStrength"] = pingPongStrength;
	data["gain"] = gain;
	data["strength"] = strength;
	data["fractalType"] = fractalType;
	return data;
}

void NoiseValueNode::OnRender()
{
	DrawHeader("Value Noise");

	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("Out");
	outputPins[0]->Render();

	inputPins[0]->Render();
	ImGui::Text("Seed");
	if (!inputPins[0]->IsLinked())
	{
		ImGui::Dummy(ImVec2(30, 10));
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::DragInt(MAKE_IMGUI_ID(inputPins[0]->id), &seed, 1);
		ImGui::PopItemWidth();
	}
	else
		ImGui::NewLine();

	inputPins[1]->Render();
	ImGui::Text("Octaves");
	if (!inputPins[1]->IsLinked())
	{
		ImGui::Dummy(ImVec2(30, 10));
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::DragInt(MAKE_IMGUI_ID(inputPins[1]->id), &octaves, 1);
		ImGui::PopItemWidth();
	}
	else
		ImGui::NewLine();

	inputPins[2]->Render();
	ImGui::Text("Frequency");
	if (!inputPins[2]->IsLinked())
	{
		ImGui::Dummy(ImVec2(30, 10));
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::DragFloat(MAKE_IMGUI_ID(inputPins[2]->id), &frequency, 0.001f);
		ImGui::PopItemWidth();
	}
	else
		ImGui::NewLine();

	inputPins[3]->Render();
	ImGui::Text("Lacunarity");
	if (!inputPins[3]->IsLinked())
	{
		ImGui::Dummy(ImVec2(30, 10));
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::DragFloat(MAKE_IMGUI_ID(inputPins[3]->id), &lacunarity, 0.01f);
		ImGui::PopItemWidth();
	}
	else
		ImGui::NewLine();
	
	inputPins[4]->Render();
	ImGui::Text("Gain");
	if (!inputPins[4]->IsLinked())
	{
		ImGui::Dummy(ImVec2(30, 10));
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::DragFloat(MAKE_IMGUI_ID(inputPins[4]->id), &gain, 0.01f);
		ImGui::PopItemWidth();
	}
	else
		ImGui::NewLine();

	inputPins[5]->Render();
	ImGui::Text("Weighted Strength");
	if (!inputPins[5]->IsLinked())
	{
		ImGui::Dummy(ImVec2(30, 10));
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::DragFloat(MAKE_IMGUI_ID(inputPins[5]->id), &weightedStrength, 0.01f, 0, 1);
		ImGui::PopItemWidth();
	}
	else
		ImGui::NewLine();

	inputPins[6]->Render();
	ImGui::Text("Ping Pong Strength");
	if (!inputPins[6]->IsLinked())
	{
		ImGui::Dummy(ImVec2(30, 10));
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::DragFloat(MAKE_IMGUI_ID(inputPins[6]->id), &pingPongStrength, 0.01f);
		ImGui::PopItemWidth();
	}
	else
		ImGui::NewLine();

	inputPins[7]->Render();
	ImGui::Text("Strength");
	if (!inputPins[7]->IsLinked())
	{
		ImGui::Dummy(ImVec2(30, 10));
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::DragFloat(MAKE_IMGUI_ID(inputPins[7]->id), &strength, 0.01f);
		ImGui::PopItemWidth();
	}
	else
		ImGui::NewLine();

	ImGui::NewLine();
	ImGui::Text("Current Fractal Type : ");
	ImGui::SameLine();
	ImGui::Text(fractalTypes[fractalType]);
	if (ImGui::Button(MAKE_IMGUI_LABEL(id, "Change Fractal Type")))
	{
		fractalType++;
		if (fractalType == 4)
			fractalType = 0;
	}
}

NoiseValueNode::NoiseValueNode()
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
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	seed = 42;
	frequency = 0.01f;
	fractalType = 0;
	octaves = 3;
	lacunarity = 2.0f;
	gain = 0.5f;
	weightedStrength = 0.0f; // should be within 0 to 1
	pingPongStrength = 2.0f;
	strength = 1.0f;
	noiseGen = new FastNoiseLite();
	noiseGen->SetNoiseType(FastNoiseLite::NoiseType::NoiseType_Value);
}

NoiseValueNode::~NoiseValueNode()
{
	delete noiseGen;
}
