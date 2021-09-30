#include <NodeEditorNodes.h>


DuplicateNode::DuplicateNode(std::string name, int id)
	: Node(name, id) {
}

void DuplicateNode::Setup() {
	inputPin.node = this;
	outputPinX.node = this;
	outputPinY.node = this;
}


std::vector<void*>  DuplicateNode::GetPins() {
	return std::vector<void*>({ &outputPinX, &outputPinY, &outputPinZ, &inputPin });
};

nlohmann::json DuplicateNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::Duplicate;
	data["outputPinX"] = outputPinX.Save();
	data["outputPinY"] = outputPinY.Save();
	data["outputPinZ"] = outputPinZ.Save();
	data["inputPin"] = inputPin.Save();
	data["id"] = id;
	data["name"] = name;
	return data;
}
void DuplicateNode::Load(nlohmann::json data) {
	inputPin.Load(data["inputPin"]);
	outputPinX.Load(data["outputPinX"]);
	outputPinY.Load(data["outputPinY"]);
	outputPinZ.Load(data["outputPinZ"]);
	id = data["id"];
	name = data["name"];
}

bool DuplicateNode::Render() {
	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(outputPinX.id);
	ImGui::Text("Output");
	ImNodes::EndOutputAttribute();

	ImNodes::BeginOutputAttribute(outputPinY.id);
	ImGui::Text("Output");
	ImNodes::EndOutputAttribute();

	ImNodes::BeginOutputAttribute(outputPinZ.id);
	ImGui::Text("Output");
	ImNodes::EndOutputAttribute();

	if (!inputPin.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPin.id);
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPin.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}

	ImNodes::EndNode();
	return true;
}

float DuplicateNode::EvaluatePin(float x, float y, int id) {
	if (inputPin.isLinked)
		return inputPin.Evaluate(x, y);
	return 0.0f;
}
