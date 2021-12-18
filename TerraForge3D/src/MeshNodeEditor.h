#pragma once

#include <json.hpp>
#include "Base/NodeEditor/NodeEditor.h"

#include "Modules/ModuleManager.h"

// Default Node Colors

#define DUMMY_NODE_COLOR          20, 31, 128
#define MATH_NODE_COLOR           11, 99, 20
#define VALUE_NODE_COLOR          11, 99, 90
#define OUTPUT_NODE_COLOR         79, 9, 32
#define OP_NODE_COLOR             39, 0, 87
#define IMAGE_NODE_COLOR          143, 95, 0
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
		MinMeshCoordinates,
		MaxMeshCoordinates,
		TextureCoordinates,
		TimeBasedSeed,
		RandomNumber,
		Duplicate,
		Add,
		Sub,
		Mul,
		Div,
		Sin,
		Cos,
		Tan,
		Abs,
		Blend,
		Curve,
		NoiseOpenSimplex2,
		NoiseOpenSimplex2S,
		NoiseCellular,
		NoisePerlin,
		NoiseValueCubic,
		NoiseValue,
		MathFunction,
		Module,
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
void SetupMeshNodeEditor(ModuleManager* manager);

void MeshNodeEditorTick();
void ShutdownMeshNodeEditor();
void ShowMeshNodeEditor(bool* pOpen);
