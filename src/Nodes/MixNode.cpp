#include <NodeEditorNodes.h>




MixNode::MixNode(std::string name, int id)
	: Node(name, id) {
}

void MixNode::Setup() {
	outputPin.node = this;
	inputPinV.node = this;
	inputPinX.node = this;
	inputPinY.node = this;
}


std::vector<void*>  MixNode::GetPins() {
	return std::vector<void*>({ &inputPinX, &inputPinY, &inputPinV, &outputPin });
};

nlohmann::json MixNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::Mix;
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
void MixNode::Load(nlohmann::json data) {
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

bool MixNode::Render() {
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
		ImGui::DragFloat((std::string("Value a##MixNode") + std::to_string(inputPinX.id)).c_str(), &a, 0.01f);
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
		ImGui::DragFloat((std::string("Value b##MixNode") + std::to_string(inputPinY.id)).c_str(), &b, 0.01f);
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
		ImGui::DragFloat((std::string("Factor##MixNode") + std::to_string(inputPinV.id)).c_str(), &factor, 0.01f, 0.0f, 1.0f);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPinV.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}

	ImNodes::EndNode();
	return true;
}

float MixNode::EvaluatePin(float x, float y, int id) {
	float aV = a;
	float bV = b;
	float factorV = factor;

	if (inputPinV.isLinked) {
		factorV = (inputPinV.Evaluate(x, y));
	}
	if (inputPinX.isLinked) {
		aV = (inputPinX.Evaluate(x, y));
	}
	if (inputPinY.isLinked) {
		bV = (inputPinY.Evaluate(x, y));
	}

	if (factorV > 1.0f)
		factorV = 1.0f;
	if (factorV < 0.0f)
		factorV = 0.0f;
	return (bV * factorV) + (aV * (1 - factorV));

}
