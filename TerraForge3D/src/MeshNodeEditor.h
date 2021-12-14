#pragma once

#include <json.hpp>

/*
* A struct to hold parameters to the mesh node editor.
* 
*/
struct MeshNodeEditorParam{
	float x;
	float y;
	float z;
};

struct MeshNodeEditorResult {
	float value;
};

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
		MeshNodeTypeCount
	};

}


// Evaluate the nodes to get the value
MeshNodeEditorResult EvaluateMeshNodeEditor(MeshNodeEditorParam param);

// Serialize Node data
nlohmann::json GetMeshNodeEditorSaveData();

// Deserialize Node data
void SetMeshNodeEditorSaveData(nlohmann::json data);

// Setup the data for mesh node editor
void SetupMeshNodeEditor(int* resolution);

void MeshNodeEditorTick();
void ShutdownMeshNodeEditor();
void ShowMeshNodeEditor(bool* pOpen);