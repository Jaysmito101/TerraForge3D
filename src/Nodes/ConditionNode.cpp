#include <NodeEditorNodes.h>




ConditionNode::ConditionNode(std::string name, int id)
	: Node(name, id) {
}

void ConditionNode::Setup() {
	outputPin.node = this;
	inputPinV.node = this;
	inputPinX.node = this;
	inputPinY.node = this;
}


std::vector<void*>  ConditionNode::GetPins() {
	return std::vector<void*>({ &inputPinX, &inputPinY, &inputPinV, &outputPin });
};

nlohmann::json ConditionNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::Condition;
	data["inputPinZ"] = inputPinV.Save();
	data["inputPinX"] = inputPinX.Save();
	data["inputPinY"] = inputPinY.Save();
	data["outputPin"] = outputPin.Save();
	data["factor"] = factor;
	data["a"] = a;
	data["b"] = b;
	data["condValue"] = condValue;
	data["id"] = id;
	data["name"] = name;
	return data;
}
void ConditionNode::Load(nlohmann::json data) {
	inputPinV.Load(data["inputPinV"]);
	inputPinX.Load(data["inputPinX"]);
	inputPinY.Load(data["inputPinY"]);
	outputPin.Load(data["outputPin"]);
	factor = data["factor"];
	a = data["a"];
	b = data["b"];
	condValue = data["condValue"];
	id = data["id"];
	name = data["name"];
}

bool ConditionNode::Render() {
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
		ImGui::DragFloat((std::string("Value a##ConditionNode") + std::to_string(inputPinX.id)).c_str(), &a, 0.01f);
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
		ImGui::DragFloat((std::string("Value b##ConditionNode") + std::to_string(inputPinY.id)).c_str(), &b, 0.01f);
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
		ImGui::DragFloat((std::string("Input##ConditionNode") + std::to_string(inputPinV.id)).c_str(), &factor, 0.01f, 0.0f, 1.0f);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPinV.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}
	ImGui::PushItemWidth(100);

	ImGui::DragFloat((std::string("Threshold##ConditionNode") + std::to_string(id)).c_str(), &condValue, 0.01f);

	ImGui::PopItemWidth();

	ImNodes::EndNode();
	return true;
}

float ConditionNode::EvaluatePin(float x, float y, int id) {
	float aV = a;
	float bV = b;
	float condInput = factor;

	if (inputPinV.isLinked) {
		condInput = (inputPinV.Evaluate(x, y));
	}
	if (inputPinX.isLinked) {
		aV = (inputPinX.Evaluate(x, y));
	}
	if (inputPinY.isLinked) {
		bV = (inputPinY.Evaluate(x, y));
	}

	if (condInput > condValue)
		return aV;
	return bV;
}
