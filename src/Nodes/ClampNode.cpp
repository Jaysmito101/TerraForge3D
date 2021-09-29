#include <NodeEditorNodes.h>



ClampNode::ClampNode(std::string name, int id)
	: Node(name, id) {
}

void ClampNode::Setup() {
	outputPin.node = this;
	inputPinV.node = this;
	inputPinX.node = this;
	inputPinY.node = this;
}


std::vector<void*>  ClampNode::GetPins() {
	return std::vector<void*>({ &inputPinX, &inputPinY, &inputPinV, &outputPin });
};

nlohmann::json ClampNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::Clamp;
	data["inputPinZ"] = inputPinV.Save();
	data["inputPinX"] = inputPinX.Save();
	data["inputPinY"] = inputPinY.Save();
	data["outputPin"] = outputPin.Save();
	data["value"] = value;
	data["min"] = min;
	data["max"] = max;
	data["id"] = id;
	data["name"] = name;
	return data;
}
void ClampNode::Load(nlohmann::json data) {
	inputPinV.Load(data["inputPinV"]);
	inputPinX.Load(data["inputPinX"]);
	inputPinY.Load(data["inputPinY"]);
	outputPin.Load(data["outputPin"]);
	value = data["value"];
	min = data["min"];
	max = data["max"];
	id = data["id"];
	name = data["name"];
}

bool ClampNode::Render() {
	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(outputPin.id);
	ImNodes::EndOutputAttribute();

	if (!inputPinV.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPinV.id);
		ImGui::PushItemWidth(100);
		ImGui::DragFloat((std::string("Value##clampNode") + std::to_string(inputPinV.id)).c_str(), &value, 0.01f);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPinV.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}

	if (!inputPinX.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPinX.id);
		ImGui::PushItemWidth(100);
		ImGui::DragFloat((std::string("Min##clampNode") + std::to_string(inputPinX.id)).c_str(), &min, 0.01f);
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
		ImGui::DragFloat((std::string("Max##clampNode") + std::to_string(inputPinY.id)).c_str(), &max, 0.01f);
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

float ClampNode::EvaluatePin(float x, float y, int id) {
	float minV = min;
	float maxV = max;
	float valV = value;

	if (inputPinV.isLinked) {
		valV = (inputPinV.Evaluate(x, y));
	}
	if (inputPinX.isLinked) {
		minV = (inputPinX.Evaluate(x, y));
	}
	if (inputPinY.isLinked) {
		maxV = (inputPinY.Evaluate(x, y));
	}
	if (valV > maxV)
		return max;
	if (valV < minV)
		return min;
	return valV;

}

