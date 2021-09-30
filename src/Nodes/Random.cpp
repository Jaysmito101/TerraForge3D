#include <NodeEditorNodes.h>



RandomNode::RandomNode(std::string name, int id)
	: Node(name, id) {
}

void RandomNode::Setup() {
	outputPin.node = this;
	inputPin.node = this;
	inputPinX.node = this;
	inputPinY.node = this;
}


std::vector<void*>  RandomNode::GetPins() {
	return std::vector<void*>({ &inputPin, &inputPinX, &inputPinY, &outputPin });
};

nlohmann::json RandomNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::Random;
	data["inputPin"] = inputPin.Save();
	data["inputPinX"] = inputPinX.Save();
	data["inputPinY"] = inputPinY.Save();
	data["outputPin"] = outputPin.Save();
	data["value"] = value;
	data["max"] = max;
	data["min"] = min;
	data["id"] = id;
	data["name"] = name;
	return data;
}
void RandomNode::Load(nlohmann::json data) {
	inputPin.Load(data["inputPin"]);
	inputPinX.Load(data["inputPinX"]);
	inputPinY.Load(data["inputPinY"]);
	outputPin.Load(data["outputPin"]);
	value = data["value"];
	max = data["max"];
	min = data["min"];
	id = data["id"];
	name = data["name"];
}

bool RandomNode::Render() {
	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(outputPin.id);
	ImNodes::EndOutputAttribute();

	if (!inputPin.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPin.id);
		ImGui::PushItemWidth(100);
		ImGui::DragInt((std::string("Seed##RandomNode") + std::to_string(inputPin.id)).c_str(), &value, 1);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPin.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}

	if (!inputPinX.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPinX.id);
		ImGui::PushItemWidth(100);
		ImGui::DragInt((std::string("Min##RandomNode") + std::to_string(inputPinX.id)).c_str(), &min, 1);
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
		ImGui::DragInt((std::string("Max##RandomNode") + std::to_string(inputPinY.id)).c_str(), &max, 1);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPinY.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}



	ImNodes::EndNode();
	return true;
}

float RandomNode::EvaluatePin(float x, float y, int id) {
	int seedV = value;
	int maxV = max;
	int minV = min;
	if (inputPin.isLinked) {
		seedV = (int)(inputPin.Evaluate(x, y));
	}
	if (inputPinX.isLinked) {
		minV = (int)(inputPinX.Evaluate(x, y));
	}
	if (inputPinY.isLinked) {
		maxV = (int)(inputPinY.Evaluate(x, y));
	}
	if (maxV < minV) {
		maxV = maxV + 1;
		minV = minV - 1;
	}
	srand(seedV);
	return (rand()%(maxV - minV + 1) + minV);
}
