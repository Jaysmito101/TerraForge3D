#include <NodeEditorNodes.h>


// ---------------------------------------------------------------------------------------------------------
// -------ABS-NODE-BEGIN------------------------------------------------------------------------------------

AbsNode::AbsNode(std::string name, int id)
	: Node(name, id) {
}

void AbsNode::Setup() {
	outputPin.node = this;
	inputPin.node = this;
}

std::vector<void*> AbsNode::GetPins() {
	return std::vector<void*>({ &inputPin, &outputPin });
};

nlohmann::json AbsNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::Abs;
	data["inputPin"] = inputPin.Save();
	data["outputPin"] = outputPin.Save();
	data["value"] = value;
	data["id"] = id;
	data["name"] = name;
	return data;
}
void AbsNode::Load(nlohmann::json data) {
	inputPin.Load(data["inputPin"]);
	outputPin.Load(data["outputPin"]);
	value = data["value"];
	id = data["id"];
	name = data["name"];
}

bool AbsNode::Render() {
	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	if (!inputPin.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPin.id);
		ImGui::PushItemWidth(100);
		ImGui::DragFloat((std::string("Value##sqrtNode") + std::to_string(inputPin.id)).c_str(), &value, 0.01f);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPin.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}

	ImNodes::BeginOutputAttribute(outputPin.id);
	ImNodes::EndOutputAttribute();


	ImNodes::EndNode();
	return true;
}

float AbsNode::EvaluatePin(float x, float y, int id) {
	if (inputPin.isLinked) {
		return abs(inputPin.Evaluate(x, y));
	}
	return abs(value);
}


// -------ABS-NODE-END--------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------


// ---------------------------------------------------------------------------------------------------------
// -------SQUARE-NODE-BEGIN---------------------------------------------------------------------------------



SquareNode::SquareNode(std::string name, int id)
	: Node(name, id) {
}

void SquareNode::Setup() {
	outputPin.node = this;
	inputPin.node = this;
}


std::vector<void*>  SquareNode::GetPins() {
	return std::vector<void*>({ &inputPin, &outputPin });
};

nlohmann::json SquareNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::Square;
	data["inputPin"] = inputPin.Save();
	data["outputPin"] = outputPin.Save();
	data["value"] = value;
	data["id"] = id;
	data["name"] = name;
	return data;
}
void SquareNode::Load(nlohmann::json data) {
	inputPin.Load(data["inputPin"]);
	outputPin.Load(data["outputPin"]);
	value = data["value"];
	id = data["id"];
	name = data["name"];
}

bool SquareNode::Render() {
	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	if (!inputPin.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPin.id);
		ImGui::PushItemWidth(100);
		ImGui::DragFloat((std::string("Value##squareNode") + std::to_string(inputPin.id)).c_str(), &value, 0.01f);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPin.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}

	ImNodes::BeginOutputAttribute(outputPin.id);
	ImNodes::EndOutputAttribute();


	ImNodes::EndNode();
	return true;
}

float SquareNode::EvaluatePin(float x, float y, int id) {
	if (inputPin.isLinked) {
		return (inputPin.Evaluate(x, y)) * (inputPin.Evaluate(x, y));
	}
	return (value * value);
}


// -------SQUARE-NODE-END-----------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------



// ---------------------------------------------------------------------------------------------------------
// -------SQRT-NODE-BEGIN-----------------------------------------------------------------------------------



SqrtNode::SqrtNode(std::string name, int id)
	: Node(name, id) {
}

void SqrtNode::Setup() {
	outputPin.node = this;
	inputPin.node = this;
}


std::vector<void*> SqrtNode::GetPins() {
	return std::vector<void*>({ &inputPin, &outputPin });
};

nlohmann::json SqrtNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::Sqrt;
	data["inputPin"] = inputPin.Save();
	data["outputPin"] = outputPin.Save();
	data["value"] = value;
	data["id"] = id;
	data["name"] = name;
	return data;
}
void SqrtNode::Load(nlohmann::json data) {
	inputPin.Load(data["inputPin"]);
	outputPin.Load(data["outputPin"]);
	value = data["value"];
	id = data["id"];
	name = data["name"];
}

bool SqrtNode::Render() {
	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	if (!inputPin.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPin.id);
		ImGui::PushItemWidth(100);
		ImGui::DragFloat((std::string("Value##sqrtNode") + std::to_string(inputPin.id)).c_str(), &value, 0.01f);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPin.id);
		ImGui::Dummy(ImVec2(100, 20));
		ImNodes::EndInputAttribute();
	}

	ImNodes::BeginOutputAttribute(outputPin.id);
	ImNodes::EndOutputAttribute();


	ImNodes::EndNode();
	return true;
}

float SqrtNode::EvaluatePin(float x, float y, int id) {
	if (inputPin.isLinked) {
		return sqrt(inputPin.Evaluate(x, y));
	}
	return sqrt(value);
}

// -------SQRT-NODE-END-------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------

