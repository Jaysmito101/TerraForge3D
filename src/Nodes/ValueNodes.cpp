#include <NodeEditorNodes.h>

// ---------------------------------------------------------------------------------------------------------
// -------MESHCOORD-NODE-BEGIN------------------------------------------------------------------------------



MeshCoordNode::MeshCoordNode(std::string name, int id)
	: Node(name, id) {
}

void MeshCoordNode::Setup() {
	outputPinX.node = this;
	outputPinY.node = this;
}

nlohmann::json MeshCoordNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::MeshCoord;
	data["outputPinX"] = outputPinX.Save();
	data["outputPinY"] = outputPinY.Save();
	data["value"] = value;
	data["id"] = id;
	data["name"] = name;
	return data;
}
void MeshCoordNode::Load(nlohmann::json data) {
	outputPinX.Load(data["outputPinX"]);
	outputPinY.Load(data["outputPinY"]);
	value = data["value"];
	id = data["id"];
	name = data["name"];
	Setup();
}

std::vector<void*> MeshCoordNode::GetPins() {
	return std::vector<void*>({ &outputPinX, &outputPinY });
};

bool MeshCoordNode::Render() {
	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(outputPinX.id);
	ImGui::Text("X");
	ImNodes::EndOutputAttribute();

	ImNodes::BeginOutputAttribute(outputPinY.id);
	ImGui::Text("Y");
	ImNodes::EndOutputAttribute();

	ImNodes::EndNode();
	return true;
}

float MeshCoordNode::EvaluatePin(float x, float y, int id) {
	if (id == outputPinX.id)
		return x;
	if (id == outputPinY.id)
		return y;
}



// -------MESHCOORD-NODE-END--------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------



// ---------------------------------------------------------------------------------------------------------
// -------FLOATVAL-NODE-BEGIN-------------------------------------------------------------------------------




FloatValueO::FloatValueO(std::string name, int id)
	: Node(name, id) {
}

void FloatValueO::Setup() {
	outputPin.node = this;
}


std::vector<void*> FloatValueO::GetPins() {
	return std::vector<void*>({ &outputPin });
};

nlohmann::json FloatValueO::Save() {
	nlohmann::json data;
	data["type"] = NodeType::FloatNodeO;
	data["outputPin"] = outputPin.Save();
	data["value"] = value;
	data["id"] = id;
	data["name"] = name;
	return data;
}
void FloatValueO::Load(nlohmann::json data) {
	outputPin.Load(data["outputPin"]);
	value = data["value"];
	id = data["id"];
	name = data["name"];
}


bool FloatValueO::Render() {
	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(outputPin.id);
	ImGui::PushItemWidth(100);
	ImGui::DragFloat((std::string("Value##floatValueNode") + std::to_string(outputPin.id)).c_str(), &value, 0.01f);
	ImGui::PopItemWidth();
	ImNodes::EndOutputAttribute();


	ImNodes::EndNode();
	return true;
}

float FloatValueO::EvaluatePin(float x, float y, int id) {
	return value;
}



// -------FLOATVAL-NODE-END---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------
