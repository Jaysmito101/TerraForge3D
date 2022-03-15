#include "Shading/ShaderNodes/ShaderTextureNode.h"

#include "Utils/Utils.h"
#include "Data/ProjectData.h"
#include "Platform.h"


#include <iostream>

void ShaderTextureNode::OnEvaluate(GLSLFunction* function, GLSLLine* line)
{
	line->line = "texture(_Textures, vec3(TexCoord, " + SDATA(0) + ")).rgb";
}

void ShaderTextureNode::Load(nlohmann::json data)
{
	if(texture != nullptr)
    {
        delete texture;
        texture = nullptr;
    }
    texture = new Texture2D(ProjectManager::Get()->GetResourcePath() + PATH_SEPERATOR + ProjectManager::Get()->GetAsset(data["texture"]));
    

    scale = data["scale"];
    offsetX = data["offsetX"];
    offsetY = data["offsetY"];
    rotation = data["rotation"];
}

nlohmann::json ShaderTextureNode::Save()
{
	nlohmann::json data;
	data["type"] = "ShaderTexture";
    data["scale"] = scale;
    data["offsetX"] = offsetX;
    data["offsetY"] = offsetY;
    data["rotation"] = rotation;
    data["texture"] = ProjectManager::Get()->SaveTexture(texture);
	return data;
}

void ShaderTextureNode::UpdateShaders()
{
    sharedData->d0 = scale;
    sharedData->d1 = offsetX;
    sharedData->d2 = offsetY;
    sharedData->d3 = rotation;
}

void ShaderTextureNode::OnRender()
{
	DrawHeader("Texture");
    if(ImGui::ImageButton((ImTextureID)texture->GetRendererID(), ImVec2(150, 150)))
    {
        std::string path = ShowOpenFileDialog("*.png\0*.jpg\0");
        if(path.size() > 3)
        {
            if(texture)
                delete texture;
            texture = new Texture2D(path);
           // update texture manager here           
        }
    }

    ImGui::SameLine();

    outputPins[0]->Render();

    ImGui::PushItemWidth(100);
    
    if(ImGui::DragFloat("Scale", &scale, 0.01f))
    {
        sharedData->d0 = scale;
    }

    if(ImGui::DragFloat("Offset X", &offsetX, 0.01f))
    {
        sharedData->d1 = offsetX;
    }

    if(ImGui::DragFloat("Offset Y", &offsetY, 0.01f))
    {
        sharedData->d2 = offsetY;
    }

    if(ImGui::DragFloat("Rotation", &rotation, 0.01f))
    {
        sharedData->d3 = rotation;
    }

    ImGui::PopItemWidth();
}

ShaderTextureNode::ShaderTextureNode(GLSLHandler* handler, ShaderTextureManager* textureManager)
	:SNENode(handler), textureManager(textureManager)
{
	name = "Texture";
	headerColor = ImColor(SHADER_TEXTURE_NODE_COLOR);
	outputPins.push_back(new SNEPin(NodeEditorPinType::Output, SNEPinType::SNEPinType_Float3));
    texture = new Texture2D(GetExecutablePath() + PATH_SEPERATOR "Data" PATH_SEPERATOR "textures" PATH_SEPERATOR "white.png");
    textureManager->Register(this);
}


ShaderTextureNode::~ShaderTextureNode()
{
    textureManager->Unregister(this);
    if(texture != nullptr)
    {
        delete texture;
        texture = nullptr;
    }
}