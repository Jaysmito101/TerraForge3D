#include <NodeEditorNodes.h>
#include <FastNoiseLite.h>

static FastNoiseLite m_NoiseGen = FastNoiseLite::FastNoiseLite(42);




PerlinNode::PerlinNode(std::string name, int id)
	: Node(name, id) {
}

void PerlinNode::Setup() {
	outputPin.node = this;
	inputPinV.node = this;
	inputPinX.node = this;
	inputPinY.node = this;
}


std::vector<void*>  PerlinNode::GetPins() {
	return std::vector<void*>({ &inputPinX, &inputPinY, &inputPinV, &outputPin });
};

nlohmann::json PerlinNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::Perlin;
	data["inputPinT"] = inputPinT.Save();
	data["inputPinZ"] = inputPinV.Save();
	data["inputPinX"] = inputPinX.Save();
	data["inputPinY"] = inputPinY.Save();
	data["outputPin"] = outputPin.Save();
	data["factor"] = factor;
	data["a"] = a;
	data["b"] = b;
	data["id"] = id;
	data["name"] = name;
	return data;
}
void PerlinNode::Load(nlohmann::json data) {
	inputPinT.Load(data["inputPinT"]);
	inputPinV.Load(data["inputPinV"]);
	inputPinX.Load(data["inputPinX"]);
	inputPinY.Load(data["inputPinY"]);
	outputPin.Load(data["outputPin"]);
	factor = data["factor"];
	a = data["a"];
	b = data["b"];
	id = data["id"];
	name = data["name"];
}

bool PerlinNode::Render() {
	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(outputPin.id);
	ImNodes::EndOutputAttribute();


	if (!inputPinX.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPinX.id);
		ImGui::PushItemWidth(100);
		ImGui::DragFloat((std::string("Scale##PerlinNode") + std::to_string(inputPinX.id)).c_str(), &a, 0.01f);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPinX.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}

	if (!inputPinY.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPinY.id);
		ImGui::PushItemWidth(100);
		ImGui::DragFloat((std::string("Offset X##PerlinNode") + std::to_string(inputPinY.id)).c_str(), &b, 1);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPinY.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}

	if (!inputPinV.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPinV.id);
		ImGui::PushItemWidth(100);
		ImGui::DragFloat((std::string("Offset Y##PerlinNode") + std::to_string(inputPinV.id)).c_str(), &factor, 1);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPinV.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}

	if (!inputPinT.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPinT.id);
		ImGui::PushItemWidth(100);
		ImGui::DragFloat((std::string("Strength##PerlinNode") + std::to_string(inputPinT.id)).c_str(), &strength, 0.01f);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPinT.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}

	ImNodes::EndNode();
	return true;
}

float PerlinNode::EvaluatePin(float x, float y, int id) {
	float scale = a;
	float offsetX = b;
	float offsetY = factor;
	float strengthV = strength;

	if (inputPinV.isLinked) {
		offsetY = (inputPinV.Evaluate(x, y));
	}
	if (inputPinX.isLinked) {
		scale = (inputPinX.Evaluate(x, y));
	}
	if (inputPinY.isLinked) {
		offsetX = (inputPinY.Evaluate(x, y));
	}
	if (inputPinT.isLinked) {
		strengthV = (inputPinT.Evaluate(x, y));
	}
	m_NoiseGen.SetNoiseType(FastNoiseLite::NoiseType::NoiseType_OpenSimplex2);

	return m_NoiseGen.GetNoise((x / 1) * scale + offsetX, (y / 1) * scale + offsetY) * strengthV;
	//return SimplexNoise::noise((x / *data.resolution) * scale + offsetX, (y / *data.resolution) * scale + offsetY) * strengthV;
}
