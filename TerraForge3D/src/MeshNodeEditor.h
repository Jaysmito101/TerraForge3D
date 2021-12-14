#pragma once

#include <json.hpp>
#include "Base/NodeEditor/NodeEditor.h"

// Default Node Colors

#define DUMMY_NODE_COLOR          20, 31, 128
#define MATH_NODE_COLOR           11, 99, 20
#define VALUE_NODE_COLOR          11, 99, 90
#define OUTPUT_NODE_COLOR         79, 9, 32
#define NOISE_NODE_COLOR          115, 90, 17


namespace MeshNodeEditor {

	enum MeshNodePinType {
		Float = 0,
		Boolean,
		Vec2,
		Vec3,
		MeshNodePinTypeCount
	};

	enum MeshNodeType {
		Dummy = 0,
		Output,
		MeshCoordinates,
		Add,
		Sub,
		Mul,
		Div,
		Sin,
		Cos,
		Tan,
		Abs,
		MeshNodeTypeCount
	};

}


// Evaluate the nodes to get the value
NodeOutput EvaluateMeshNodeEditor(NodeInputParam param);

// Serialize Node data
nlohmann::json GetMeshNodeEditorSaveData();

// Deserialize Node data
void SetMeshNodeEditorSaveData(nlohmann::json data);

// Setup the data for mesh node editor
void SetupMeshNodeEditor(int* resolution);

void MeshNodeEditorTick();
void ShutdownMeshNodeEditor();
void ShowMeshNodeEditor(bool* pOpen);