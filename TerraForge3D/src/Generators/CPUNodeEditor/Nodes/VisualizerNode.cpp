#include "Generators/CPUNodeEditor/Nodes/VisualizerNode.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include "Utils.h"
#include "implot.h"
#include <iostream>

#include <mutex>
#include "Base/ImGuiCurveEditor.h"

NodeOutput VisualizerNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	inputC = input;

	if (inputPins[0]->IsLinked())
	{
		return inputPins[0]->other->Evaluate(input);
	}

	return NodeOutput({ 0.0f });
}

void VisualizerNode::Load(nlohmann::json data)
{
}

nlohmann::json VisualizerNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Visualizer;
	return data;
}

void VisualizerNode::OnRender()
{
	DrawHeader("Visualizer");
	inputPins[0]->Render();
	ImGui::Text("In");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("Out");
	outputPins[0]->Render();
	ImGui::NewLine();

	if (ImGui::Button("Update"))
	{
		if (outputPins[0]->IsLinked())
		{
			map.clear();
			mutex.lock();
			bool isIl = inputPins[0]->IsLinked();
			NodeInputParam p;

			for (int i = 0; i < inputC.maxX; i++)
			{
				for (int j = 0; j < inputC.maxY; j++)
				{
					if (isIl)
					{
						p = inputC; p.y = (float)i; p.x = (float)j;
						map.push_back(inputPins[0]->other->Evaluate(p).value);
					}
					else map.push_back(0.0f);
				}
			}

			mutex.unlock();
		}

		else
		{
			ShowMessageBox("Output pin must be connected in order to visualize", "Error");
		}
	}

	ImGui::NewLine();
	ImGui::Text("Right Click to visualize!");

	if (map.size() > 0)
	{
		ImGuiNodeEditor::Suspend();

		if (ImGui::BeginPopupContextWindow(MAKE_IMGUI_LABEL(id, "Plot")))
		{
			ImGui::BeginChild(MAKE_IMGUI_LABEL(id, "PlotChild"), ImVec2(300, 300));

			if (ImPlot::BeginPlot(MAKE_IMGUI_ID(id)))
			{
				ImPlot::PlotHeatmap<float>(MAKE_IMGUI_ID(outputPins[0]->id), map.data(), (int)inputC.maxY, (int)inputC.maxX);
				ImPlot::EndPlot();
			}

			ImGui::EndChild();
			ImGui::EndPopup();
		}

		ImGuiNodeEditor::Resume();
	}
}

VisualizerNode::VisualizerNode()
{
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(OP_NODE_COLOR);
	map.clear();
}