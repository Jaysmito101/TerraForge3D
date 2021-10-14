#include <NodeEditorNodes.h>
#include <Utils.h>
#include <ProjectData.h>


float TextureSamplerNode::EvaluatePin(float x, float y, int id)  {
	if(!texture)
		return 0.0f;
	if (!texture->GetData())
		return 0.0f;
	if (!data.resolution)
		return 0.0f;

	unsigned char sampledValue = 0;
	if(id == outputPinR.id)
		sampledValue = (texture->GetData()[(int)x * (*(data.resolution)) * 3 + (int)y * 3 + 0]);
	if (id == outputPinG.id)
		sampledValue = (texture->GetData()[(int)x * (*(data.resolution)) * 3 + (int)y * 3 + 1]);
	if (id == outputPinB.id)
		sampledValue = (texture->GetData()[(int)x * (*(data.resolution)) * 3 + (int)y * 3 + 2]);
	float sampleTResult = (sampledValue / 256.0f)*2.0f-1.0f;
	return sampleTResult;
}

void TextureSamplerNode::Setup() {
	outputPinR.node = this;
	outputPinG.node = this;
	outputPinB.node = this;
}

nlohmann::json TextureSamplerNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::TextureSampler2D;
	data["outputPinR"] = outputPinR.Save();
	data["outputPinG"] = outputPinG.Save();
	data["outputPinB"] = outputPinB.Save();
	data["textureFilePath"] = textureFilePath;
	data["id"] = id;
	data["name"] = name;
	return data;
}
void TextureSamplerNode::Load(nlohmann::json data) {
	outputPinR.Load(data["outputPinR"]);
	outputPinG.Load(data["outputPinG"]);
	outputPinB.Load(data["outputPinB"]);
	Setup();
	textureFilePath = data["textureFilePath"];
	if (textureFilePath.size() > 0) {
		if (texture)
			delete texture;
		Log("Loaded texture with ID : " + textureFilePath);
		texture = new Texture2D(GetProjectResourcePath() + "\\" + GetProjectAsset(textureFilePath));
	}
	else {
		texture = new Texture2D(GetExecutableDir() + "\\Data\\textures\\white.png");
	}
	id = data["id"];
	name = data["name"];
}

std::vector<void*>  TextureSamplerNode::GetPins() {
	return std::vector<void*>({ &outputPinR, &outputPinG, &outputPinB });
};

bool TextureSamplerNode::Render()  {
	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::TextUnformatted((name).c_str());
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(outputPinR.id);
	ImGui::TextColored(ImVec4(0.7f, 0.2f, 0.2f, 1.0f), "R");
	ImNodes::EndOutputAttribute();

	ImNodes::BeginOutputAttribute(outputPinG.id);
	ImGui::TextColored(ImVec4(0.2f, 0.7f, 0.2f, 1.0f), "G");
	ImNodes::EndOutputAttribute();

	ImNodes::BeginOutputAttribute(outputPinB.id);
	ImGui::TextColored(ImVec4(0.2f, 0.2f, 0.7f, 1.0f), "B");
	ImNodes::EndOutputAttribute();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));

	if (!isLoaded) {
		if (ImGui::Button("Load Texture")) {
			textureFilePath = ShowOpenFileDialog((wchar_t*)L".png\0");
			if (textureFilePath.size() > 1) {
				if (texture)
					delete texture;
				std::string uid = GenerateId(64);
				if (!PathExist(GetProjectResourcePath() + "\\textures"))
					MkDir(GetProjectResourcePath() + "\\textures");
				CopyFileData(textureFilePath, GetProjectResourcePath() + "\\textures\\" + uid);
				RegisterProjectAsset(uid, "textures\\" + uid);
				Log("Loaded texture with ID : " + uid);
				textureFilePath = uid;
				texture = new Texture2D(GetProjectResourcePath() + "\\textures\\" + uid);
				isLoaded = true;
				if(data.resolution)
					texture->Resize(*data.resolution, *data.resolution);
			}
			else {
				isLoaded = false;
				textureFilePath = "";
			}
		}
	}
	else {
		if (ImGui::Button("Change Texture")) {
			textureFilePath = ShowOpenFileDialog((wchar_t*)L".png\0");
			if (textureFilePath.size() > 1) {
				if (texture)
					delete texture;
				std::string uid = GenerateId(64);
				if (!PathExist(GetProjectResourcePath() + "\\textures"))
					MkDir(GetProjectResourcePath() + "\\textures");
				CopyFileData(textureFilePath, GetProjectResourcePath() + "\\textures\\" + uid);
				RegisterProjectAsset(uid, "textures\\" + uid);
				Log("Loaded texture with ID : " + uid);
				textureFilePath = uid;
				texture = new Texture2D(GetProjectResourcePath() + "\\textures\\" + uid);
				isLoaded = true;
				if (data.resolution)
					texture->Resize(*data.resolution, *data.resolution);
			}
			else {
				isLoaded = false;
				textureFilePath = "";
			}
		}
	}

	ImGui::PopStyleVar();

	if (texture) {
		ImGui::Image((ImTextureID)texture->GetRendererID(), ImVec2(200, 200));
	}

	ImNodes::EndNode();


	return true;
}