#include <NodeEditorNodes.h>
#include <GLFW/glfw3.h>

TimeNode::TimeNode(std::string name, int id)
	: Node(name, id) {
}

void TimeNode::Setup() {
	outputPin.node = this;
}


std::vector<void*> TimeNode::GetPins() {
	return std::vector<void*>({ &outputPin });
};

nlohmann::json TimeNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::Time;
	data["outputPin"] = outputPin.Save();
	data["id"] = id;
	data["name"] = name;
	return data;
}
void TimeNode::Load(nlohmann::json data) {
	outputPin.Load(data["outputPin"]);
	id = data["id"];
	name = data["name"];
}


bool TimeNode::Render() {
	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(outputPin.id);
	ImGui::Text("Time: ");
	ImNodes::EndOutputAttribute();


	ImNodes::EndNode();
	return true;
}

float TimeNode::EvaluatePin(float x, float y, int id) {
	return (float)glfwGetTime();
}
