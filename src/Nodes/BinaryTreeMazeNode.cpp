#include <NodeEditorNodes.h>

BinaryTreeMazeNode::BinaryTreeMazeNode(std::string name, int id)
	: Node(name, id) {}

void BinaryTreeMazeNode::Setup() {
	outputPin.node = this;
}

std::vector<void*> BinaryTreeMazeNode::GetPins() {
	return std::vector<void*>({ &outputPin });
}

nlohmann::json BinaryTreeMazeNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::BinTreeMaze;
	data["outputPin"] = outputPin.Save();
	data["seed"] = seed;
	return data;
}

void BinaryTreeMazeNode::Load(nlohmann::json data) {
	outputPin.Load(data["outputPin"]);
	seed = data["seed"];
}

float BinaryTreeMazeNode::EvaluatePin(float x, float y, int id) {
	static int gridSize = 0;
	static int oseed = 0;
	if (data.resolution) {
		if (gridSize != *data.resolution || !grid || oseed != seed) {
			if (grid)
				delete[] grid;
			oseed = seed;
			gridSize = *data.resolution;
			grid = new bool[gridSize * gridSize];
			memset(grid, 0, sizeof(bool) * gridSize * gridSize);
			srand(seed);
			for (int i = 0; i < gridSize; i+=2) {
				for (int j = 0; j < gridSize; j++) {
					if (i != 0 && j != 0) {
						if ((rand() % 10) > 5)
							grid[(i - 1) * gridSize + j] = true;
						else
							grid[i * gridSize + (j - 1)] = true;
					}
				}
			}
		}
	}
	else {
		if (grid)
			delete[] grid;
		grid = nullptr;
	}
	if (grid)
		return grid[(int)x * gridSize + (int)y] ? 1.0f : 0.0f;
	return 0.0f;
}

bool BinaryTreeMazeNode::Render() {
	ImGui::PushItemWidth(100);
	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::Text(name.c_str());
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(outputPin.id);
	ImNodes::EndOutputAttribute();

	ImGui::DragInt((std::string("##sizeBinMazeNode") + std::to_string(id)).c_str(), &seed, 1);

	ImNodes::EndNode();
	ImGui::PopItemWidth();
	return false;
}