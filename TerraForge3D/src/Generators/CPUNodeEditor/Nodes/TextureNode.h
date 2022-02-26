#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class Texture2D;

class TextureNode : public NodeEditorNode {
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin* pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	TextureNode();
	~TextureNode();

	Texture2D* texture;
	float scale;
	bool isDefault;
	bool autoTiled;
    bool inv;
    bool npScale;
	int numTiles;
	float posi[2];
	
private:
	void ChangeTexture();
	std::mutex mutex;
};

