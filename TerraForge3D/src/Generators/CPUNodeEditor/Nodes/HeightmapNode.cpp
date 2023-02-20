#include "Generators/CPUNodeEditor/Nodes/HeightmapNode.h"
#include "Utils/Utils.h"
#include "Data/ProjectData.h"
#include "Base/Heightmap.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>
#include <implot.h>
#include <mutex>
#include <cmath>
#include "Base/ImGuiCurveEditor.h"
#include "Platform.h"

static float fract(float v)
{
	return v - floorf(v);
}

NodeOutput HeightmapNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	if (isDefault)
		return NodeOutput({0.0f});
	float res, sc, x, y;
	res = sc = x = y = 0.0f;
	int channel = 0;

	if (inputPins[0]->IsLinked())
	{
		x = inputPins[0]->other->Evaluate(input).value;
	}

	else
	{
		x = input.texX;
	}

	if (inputPins[1]->IsLinked())
	{
		y = inputPins[1]->other->Evaluate(input).value;
	}

	else
	{
		y = input.texY;
	}

	if (inputPins[2]->IsLinked())
	{
		sc = inputPins[2]->other->Evaluate(input).value;
	}

	else
	{
		sc = scale;
	}

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
	x = fract(x);
	y = fract(y);
	int xC = (int)(x * (heightmap->GetWidth()-1));
	int yC = (int)(y * (heightmap->GetHeight()-1));


	mutex.lock();

	res = heightmap->Sample(x, y, interpolated) / 65536.0f;
	if (!autoTiled)
	{
		if(x > numTiles || y > numTiles || x < 0 || y < 0)
			return NodeOutput({ 0.0f });
	}

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

void HeightmapNode::Load(nlohmann::json data)
{
	scale = data["scale"];

	if (isDefault && data["isDefault"])
	{
		return;
	}

	isDefault = data["isDefault"];
	interpolated = data["interpolated"];
	npScale = data["npsc"];
	inv = data["inv"];
	autoTiled = data["autoTiled"];
	numTiles = data["numTiles"];
	posi[0] = data["posiX"];
	posi[1] = data["posiY"];
	rota = data["rota"];

	if (isDefault)
	{
		delete heightmap;
		heightmap = new Heightmap(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "heightmaps" PATH_SEPARATOR "flat.png");
	}

	else
	{
		std::string hash = data["heightmap"];

		if (!ProjectManager::Get()->AssetExists(hash))
		{
			ShowMessageBox("Failed to Load Heightmap : " + hash, "Error");
			isDefault = true;
		}

		else
		{
			delete heightmap;
			heightmap = new Heightmap(ProjectManager::Get()->GetResourcePath() + PATH_SEPARATOR + ProjectManager::Get()->GetAsset(hash));
			Log("Loaded Cached Heightmap : " + hash);
		}
	}
}

nlohmann::json HeightmapNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Heightmap;
	data["scale"] = scale;
	data["isDefault"] = isDefault;
	data["interpolated"] = interpolated;
	data["inv"] = inv;
	data["npsc"] = npScale;
	data["autoTiled"] = autoTiled;
	data["numTiles"] = numTiles;
	data["posiX"] = posi[0];
	data["posiY"] = posi[1];
	data["rota"] = rota;

	if (!isDefault)
	{
		std::string hash = MD5File(heightmap->GetPath()).ToString();
		data["heightmap"] = hash;

		if (!ProjectManager::Get()->AssetExists(hash))
		{
			ProjectManager::Get()->SaveResource("heightmaps", heightmap->GetPath());
			Log("Cached " + heightmap->GetPath());
		}

		else
		{
			Log("Heightmap already cached : " + hash);
		}
	}

	return data;
}

void HeightmapNode::OnRender()
{
	DrawHeader("Heightmap");
	inputPins[0]->Render();
	ImGui::Text("X");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::Text("Height");
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	outputPins[0]->Render();
	inputPins[1]->Render();
	ImGui::Text("Y");
	inputPins[2]->Render();

	if (inputPins[2]->IsLinked())
	{
		ImGui::Text("Scale");
	}

	else
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("Scale##" + std::to_string(inputPins[1]->id)).c_str(), &scale, 0.01f));
		ImGui::PopItemWidth();
	}

	UPDATE_HAS_CHHANGED(ImGui::Checkbox(("Auto Tiled##tild" + std::to_string(id)).c_str(), &autoTiled));
	UPDATE_HAS_CHHANGED(ImGui::Checkbox(("Interpolated##intrp" + std::to_string(id)).c_str(), &interpolated));
	UPDATE_HAS_CHHANGED(ImGui::Checkbox(("Inverted##tinv" + std::to_string(id)).c_str(), &inv));
	UPDATE_HAS_CHHANGED(ImGui::Checkbox(("Scale -1 To 1##tnpsc" + std::to_string(id)).c_str(), &npScale));

	if(!autoTiled)
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("Num Tiles##nmtl" + std::to_string(id)).c_str(), &numTiles, 0.01f));
		ImGui::PopItemWidth();
	}
	ImGui::PushItemWidth(100);
	UPDATE_HAS_CHHANGED(ImGui::DragFloat2(("Position##posi" + std::to_string(id)).c_str(), posi, 0.01f));
	UPDATE_HAS_CHHANGED(ImGui::DragFloat(("Rotation##rota" + std::to_string(id)).c_str(), &rota, 0.1f));
	ImGui::PopItemWidth();
	ImGui::NewLine();

	if (ImGui::ImageButton((ImTextureID)(uint64_t)heightmap->GetRendererID(), ImVec2(200, 200)))
	{
		ChangeHeightmap();
		hasChanged = true;
	}

	if (ImGui::Button(MAKE_IMGUI_LABEL(id, "Change Heightmap")))
	{
		ChangeHeightmap();
		hasChanged = true;
	}
}

void HeightmapNode::ChangeHeightmap()
{
	std::string path = ShowOpenFileDialog(".png");

	if (path.size() < 3)
	{
		return;
	}

	isDefault = false;
	delete heightmap;
	heightmap = new Heightmap(path);
	if (heightmap->GetData())
	{
		Log("Changed Heightmap : " + heightmap->GetPath());
	}
	else {
		Log("Failed To Change Heightmap : " + heightmap->GetPath());
	}
}


HeightmapNode::~HeightmapNode()
{
	if(heightmap)
	{
		delete heightmap;
	}
}

HeightmapNode::HeightmapNode()
{
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(IMAGE_NODE_COLOR);
	heightmap = new Heightmap(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "heightmaps" PATH_SEPARATOR "flat.png");
	isDefault = true;
	scale = 1.0f;
	inv = false;
	npScale = true;
	autoTiled = true;
	interpolated = true;
	numTiles = 1;
	rota = 0.0f;
	posi[0] = posi[1] = 0.0f;
}
