#include "Generators/CPUNodeEditor/Nodes/TextureNode.h"
#include "Utils/Utils.h"
#include "Data/ProjectData.h"
#include "Base/Texture2D.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>
#include <implot.h>
#include <mutex>
#include <cmath>
#include "Base/ImGuiCurveEditor.h"
#include "Platform.h"

#define CLAMP01(x) x > 1 ? 1 : ( x < 0 ? 0 : x )

static float fract(float v)
{
	return v - floorf(v);
}

NodeOutput TextureNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	if (isDefault)
		return NodeOutput({0.0f});
	float res, sc, x, y;
	res = sc = x = y = 0.0f;
	int channel = 0;

	if (pin->id == outputPins[0]->id) channel = 0;
	else if (pin->id == outputPins[1]->id) channel = 1;
	else if (pin->id == outputPins[2]->id) channel = 2;

	if (inputPins[0]->IsLinked()) x = inputPins[0]->other->Evaluate(input).value;
	else x = input.texX;

	if (inputPins[1]->IsLinked()) y = inputPins[1]->other->Evaluate(input).value;
	else y = input.texY;

	if (inputPins[2]->IsLinked()) sc = inputPins[2]->other->Evaluate(input).value;
	else sc = scale;

	x = (x * 2.0f - 1.0f) * sc - posi[0];
	y = (y * 2.0f - 1.0f) * sc - posi[1];
	float cr = cos(rota * 3.1415926535f / 180.0f);
	float sr = sin(rota * 3.1415926535f / 180.0f);
	float tx = x;
	float ty = y;
	x = tx * cr - ty * sr;
	y = tx * sr + ty * cr;
	x = x * 0.5f + 0.5f;
	y = y * 0.5f + 0.5f;

	if(!autoTiled)
	{
		if(x > numTiles || y > numTiles || x < 0 || y < 0)
			return NodeOutput({ 0.0f });
	}

	x = fract(x);
	y = fract(y);
	mutex.lock();
	int xC = (int)(x * (texture->GetWidth()-1));
	int yC = (int)(y * (texture->GetHeight()-1));
	unsigned char elevC = texture->GetData()[yC * texture->GetWidth() * 3 + xC * 3 + channel];
	res = (float)elevC / 256;

	if(npScale)
	{
		res = res * 2.0f - 1.0f;
	}

	if(inv)
	{
		res = 1.0f - res;
	}

	mutex.unlock();
	return NodeOutput({ res });
}

void TextureNode::Load(nlohmann::json data)
{
	scale = data["scale"];

	if (isDefault && data["isDefault"])
	{
		return;
	}

	isDefault = data["isDefault"];
	npScale = data["npsc"];
	inv = data["inv"];
	autoTiled = data["autoTiled"];
	numTiles = data["numTiles"];
	posi[0] = data["posiX"];
	posi[1] = data["posiY"];
	rota = data["rota"];

	if (isDefault)
	{
		delete texture;
		texture = new Texture2D(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "textures" PATH_SEPARATOR "white.png", false, false);
	}

	else
	{
		std::string hash = data["texture"];

		if (!ProjectManager::Get()->AssetExists(hash))
		{
			ShowMessageBox("Failed to Load Texture : " + hash, "Error");
			isDefault = true;
		}

		else
		{
			delete texture;
			texture = new Texture2D(ProjectManager::Get()->GetResourcePath() + PATH_SEPARATOR + ProjectManager::Get()->GetAsset(hash));
			Log("Loaded Cached Texture : " + hash);
		}
	}

	// TODO : Load Image
}

nlohmann::json TextureNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Texture;
	data["scale"] = scale;
	data["isDefault"] = isDefault;
	data["inv"] = inv;
	data["npsc"] = npScale;
	data["autoTiled"] = autoTiled;
	data["numTiles"] = numTiles;
	data["posiX"] = posi[0];
	data["posiY"] = posi[1];
	data["rota"] = rota;

	if (!isDefault)
	{
		std::string hash = MD5File(texture->GetPath()).ToString();
		data["texture"] = hash;

		if (!ProjectManager::Get()->AssetExists(hash))
		{
			ProjectManager::Get()->SaveTexture(texture);
			Log("Cached " + texture->GetPath());
		}

		else
		{
			Log("Texture already Cached : " + hash);
		}
	}

	return data;
}

void TextureNode::OnRender()
{
	DrawHeader("Texture");
	inputPins[0]->Render();
	ImGui::Text("X");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("R");
	outputPins[0]->Render();
	inputPins[1]->Render();
	ImGui::Text("Y");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("G");
	outputPins[1]->Render();
	inputPins[2]->Render();

	if (inputPins[2]->IsLinked())
	{
		ImGui::Text("Scale");
	}

	else
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("##" + std::to_string(inputPins[1]->id)).c_str(), &scale, 0.01f));
		ImGui::PopItemWidth();
	}

	ImGui::SameLine();
	ImGui::Dummy(ImVec2(60, 10));
	ImGui::SameLine();
	ImGui::Text("B");
	outputPins[2]->Render();
	ImGui::NewLine();
	UPDATE_HAS_CHHANGED(ImGui::Checkbox(("Auto Tiled##tild" + std::to_string(id)).c_str(), &autoTiled));
	UPDATE_HAS_CHHANGED(ImGui::Checkbox(("Inverse Texture##tinv" + std::to_string(id)).c_str(), &inv));
	UPDATE_HAS_CHHANGED(ImGui::Checkbox(("Scale -1 To 1##tnpsc" + std::to_string(id)).c_str(), &npScale));


	ImGui::PushItemWidth(100);
	if(!autoTiled) UPDATE_HAS_CHHANGED(ImGui::DragFloat(("Num Tiles##nmtl" + std::to_string(id)).c_str(), &numTiles, 0.01f));
	UPDATE_HAS_CHHANGED(ImGui::DragFloat2(("Position##posi" + std::to_string(id)).c_str(), posi, 0.01f));
	UPDATE_HAS_CHHANGED(ImGui::DragFloat(("Rotation##rota" + std::to_string(id)).c_str(), &rota, 0.1f));
	ImGui::PopItemWidth();
	ImGui::NewLine();

	if (ImGui::ImageButton((ImTextureID)(uint64_t)texture->GetRendererID(), ImVec2(200, 200)))
	{
		ChangeTexture();
		hasChanged = true;
	}

	if (ImGui::Button(MAKE_IMGUI_LABEL(id, "Change Texture")))
	{
		ChangeTexture();
		hasChanged = true;
	}
}

void TextureNode::ChangeTexture()
{
	std::string path = ShowOpenFileDialog(".png");

	if (path.size() < 3)
	{
		return;
	}

	isDefault = false;
	delete texture;
	texture = new Texture2D(path, true, false);
	texture->Resize(256, 256);
	Log("Loaded Texture : " + texture->GetPath());
}


TextureNode::~TextureNode()
{
	if(texture)
	{
		delete texture;
	}
}

TextureNode::TextureNode()
{
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(IMAGE_NODE_COLOR);
	texture = new Texture2D(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "textures" PATH_SEPARATOR "white.png", false, false);
	isDefault = true;
	scale = 1.0f;
	inv = false;
	npScale = true;
	autoTiled = true;
	numTiles = 1;
	rota = 0.0f;
	posi[0] = posi[1] = 0.0f;
}
