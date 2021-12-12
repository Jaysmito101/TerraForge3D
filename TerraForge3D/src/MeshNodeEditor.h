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