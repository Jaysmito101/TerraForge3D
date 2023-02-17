#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include "Data/ApplicationState.h"
#include "Base/Heightmap.h"
#include <vector>

class Heightmap;

class HeightmapNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	HeightmapNode();

	~HeightmapNode();

	Heightmap *heightmap;
	float scale;
	bool isDefault;
	bool autoTiled;
    bool interpolated;
	bool inv;
	bool npScale;
	float numTiles;
	float posi[2];
	float rota;

private:
	void ChangeHeightmap();
	std::mutex mutex;
};

