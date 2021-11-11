#include <NodeEditorNodes.h>

GeneratorNode::GeneratorNode(std::string name, int id)
	: Node(name, id) {
}

void GeneratorNode::Setup() {
	outputPin.node = this;
	inputPinV.node = this;
	inputPinX.node = this;
	inputPinY.node = this;
	inputPinPlacer.node = this;
	inputPinTiles.node = this;
}


std::vector<void*> GeneratorNode::GetPins() {
	return std::vector<void*>({ &inputPinX, &inputPinY, &inputPinV, &inputPinTiles, &inputPinPlacer, &outputPin });
};

nlohmann::json GeneratorNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::Generator;
	data["inputPinPlacer"] = inputPinPlacer.Save();
	data["outputPin"] = outputPin.Save();
	data["gridSize"] = gridSize;

	nlohmann::json gridData;
	for (int i = 0; i < gridSize * gridSize; i++) {
		gridData.push_back(grid[i]);
	}
	data["gridData"] = gridData;


	data["id"] = id;
	data["name"] = name;
	return data;
}
void GeneratorNode::Load(nlohmann::json data) {
	outputPin.Load(data["outputPin"]);
	inputPinPlacer.Load(data["inputPinPlacer"]);
	gridSize = data["gridSize"];
	if (grid)
		delete[] grid;
	grid = new float[gridSize * gridSize];
	int i = 0;
	for (float v : data["gridData"]) {
		grid[i++] = v;
	}

	id = data["id"];
	name = data["name"];
}


bool GeneratorNode::Render() {

	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(outputPin.id);
	ImNodes::EndOutputAttribute();

	ImNodes::BeginInputAttribute(inputPinPlacer.id);
	ImGui::Text("Inpput Placement");
	ImNodes::EndInputAttribute();

	ImGui::PushItemWidth(100);
	ImGui::DragInt((std::string("Grid Size##RandomNode") + std::to_string(id)).c_str(), &gridSize, 1);
	ImGui::DragFloat((std::string("Smoothness##RandomNode") + std::to_string(id)).c_str(), &smoothness, 0.01f);
	ImGui::PopItemWidth();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));

	if (ImGui::Button("Generate")) {
		if (inputPinPlacer.isLinked && inputPinPlacer.node->name.find("Maze") != std::string::npos)
			inputPinPlacer.node->data.resolution = &gridSizeOld;
		for (int i = 0; i < gridSize; i++) {
			for (int j = 0; j < gridSize; j++) {
				grid[i * gridSize + j] = inputPinPlacer.Evaluate(i, j);
			}
		}
		std::cout << ("Generated Generator Data.\n");
	}

	ImGui::PopStyleVar();

	ImGui::NewLine();

	ImGui::Text("Grid:");

	if (grid) {
		if (gridSizeOld != gridSize) {
			delete[] grid;
			grid = new float[gridSize * gridSize];
			gridSizeOld = gridSize;
			for (int i = 0; i < gridSize * gridSize; i++)
				grid[i] = 0;
		}
	}
	else {
		grid = new float[gridSize * gridSize];
		gridSizeOld = gridSize;
		for (int i = 0; i < gridSize * gridSize; i++)
			grid[i] = 0;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 1));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

	float maxH = 1.0;
	float minH = 0.0;

	for (int i = 0; i < gridSize; i++) {
		for (int j = 0; j < gridSize; j++) {
			if (maxH < grid[i * gridSize + j])
				maxH = grid[i * gridSize + j];
			if (minH > grid[i * gridSize + j])
				minH = grid[i * gridSize + j];
		}
	}

	for (int i = 0; i < gridSize; i++) {
		for (int j = 0; j < gridSize; j++) {
			float color = (grid[i * gridSize + j] - minH) / maxH;
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color, color, color, 1));
			if (ImGui::Button((std::string("  ##i") + std::to_string(i) + "_j" + std::to_string(j)).c_str())) {
				grid[i * gridSize + j] = grid[i * gridSize + j] < 1 ? 1 : 0;
			}
			ImGui::PushItemWidth(100);
			if (ImGui::BeginPopupContextItem()) {
				ImGui::DragFloat((std::string("##HtIti") + std::to_string(i) + "_j" + std::to_string(j)).c_str(), &grid[i * gridSize + j], 0.1f);
				ImGui::EndPopup();
			}
			ImGui::PopItemWidth();
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
		float retVal = grid[i * gridSizeOld + j];
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
