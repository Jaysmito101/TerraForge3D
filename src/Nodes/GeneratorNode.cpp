#include <NodeEditorNodes.h>

GeneratorNode::GeneratorNode(std::string name, int id)
	: Node(name, id) {
}

void GeneratorNode::Setup() {
	outputPin.node = this;
	inputPinV.node = this;
	inputPinX.node = this;
	inputPinY.node = this;
}


std::vector<void*> GeneratorNode::GetPins() {
	return std::vector<void*>({ &inputPinX, &inputPinY, &inputPinV, &outputPin });
};

nlohmann::json GeneratorNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::Cellular;
	data["inputPinT"] = inputPinT.Save();
	data["inputPinZ"] = inputPinV.Save();
	data["inputPinX"] = inputPinX.Save();
	data["inputPinY"] = inputPinY.Save();
	data["outputPin"] = outputPin.Save();

	data["id"] = id;
	data["name"] = name;
	return data;
}
void GeneratorNode::Load(nlohmann::json data) {

}


bool GeneratorNode::Render() {

	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(outputPin.id);
	ImNodes::EndOutputAttribute();

	ImGui::PushItemWidth(100);
	ImGui::DragInt((std::string("Grid Size##RandomNode") + std::to_string(inputPinT.id)).c_str(), &gridSize, 1);
	ImGui::DragFloat((std::string("Smoothness##RandomNode") + std::to_string(inputPinV.id)).c_str(), &smoothness, 0.01f);
	ImGui::PopItemWidth();

	ImGui::NewLine();

	ImGui::Text("Grid:");

	if (grid) {
		if (gridSizeOld != gridSize) {
			delete[] grid;
			grid = new bool[gridSize * gridSize];
			gridSizeOld = gridSize;
			for (int i = 0; i < gridSize * gridSize; i++)
				grid[i] = false;
		}
	}
	else {
		grid = new bool[gridSize * gridSize];
		gridSizeOld = gridSize;
		for (int i = 0; i < gridSize * gridSize; i++)
			grid[i] = false;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 1));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
	for (int i = 0; i < gridSize; i++) {
		for (int j = 0; j < gridSize; j++) {
			if (grid[i * gridSize + j]) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 1));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 1));
			}
			if (ImGui::Button((std::string("  ##i") + std::to_string(i) + "_j" + std::to_string(j)).c_str())) {
				grid[i * gridSize + j] = !grid[i * gridSize + j];
			}
			ImGui::PopStyleColor();
			ImGui::SameLine();
		}
		ImGui::NewLine();
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();


	ImNodes::EndNode();
	return true;
}

float GeneratorNode::EvaluatePin(float x, float y, int id) {
	if (grid && gridSize >= 1) {
		int i = (int)((x / (*data.resolution)) * gridSizeOld);
		int j = (int)((y / (*data.resolution)) * gridSizeOld);
		float retVal = grid[i * gridSizeOld + j] ? 0.8f : 0.0f;
		float diffX = abs(i - ((x / (*data.resolution)) * gridSizeOld)) - 0.5f;
		float diffY = abs(j - ((y / (*data.resolution)) * gridSizeOld)) - 0.5f;
		float t = diffX * diffX + diffY * diffY;
		t = sqrt(t)*(1-smoothness);
		t = 1 - t;
		retVal = retVal * (t);
		return retVal;
	}
	return 0.0f;
}
