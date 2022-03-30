#pragma once

#include "json/json.hpp"

#include <string>
#include <mutex>

#include "Base/NodeEditor/NodeEditor.h"

#define DUMMY_NODE_COLOR          20, 31, 128
#define MATH_NODE_COLOR           11, 99, 20
#define VALUE_NODE_COLOR          11, 99, 90
#define OUTPUT_NODE_COLOR         79, 9, 32
#define OP_NODE_COLOR             39, 0, 87
#define IMAGE_NODE_COLOR          143, 95, 0
#define NOISE_NODE_COLOR          115, 90, 17

struct ApplicationState;

namespace CPUNodeEditorE
{

enum CPUNodePinType
{
	Float = 0,
	Boolean,
	Vec2,
	Vec3,
	MeshNodePinTypeCount
};

enum CPUNodeType
{
	Dummy = 0,
	Output,
	MeshCoordinates,
	MinMeshCoordinates,
	MaxMeshCoordinates,
	TextureCoordinates,
	TimeBasedSeed,
	RandomNumber,
	Duplicate,
	MinVal,
	Add,
	Sub,
	Mul,
	Div,
	Sin,
	Square,
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
	Pixelate,
	Texture,
	Heightmap,
	RectangleMask,
	Visualizer,
	Hill,
	Clamp,
	MeshNodeTypeCount
};


}

// Temporary Fix
#define MeshNodeEditor CPUNodeEditorE
#define MeshNodeType CPUNodeType

class CPUNodeEditor
{
public:
	CPUNodeEditor(ApplicationState *appState);
	~CPUNodeEditor();

	nlohmann::json Save();
	void Load(nlohmann::json data);

	void ShowSetting(int id);

	void Update();

	float EvaluateAt(NodeInputParam param);

	bool windowStat = false;
	bool uiActive = false;
	bool enabled = true;
	double time = 0;
	std::string uid;
	std::string name;
	ApplicationState *appState;
	NodeEditor *editor;

	std::mutex m;
};