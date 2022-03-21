#include "Shading/ShaderNodes/ShaderTextureNode.h"

#include "Utils/Utils.h"
#include "Data/ProjectData.h"
#include "Platform.h"


#include <iostream>

static GLSLFunction GetTriplanarBlendFunction()
{
	GLSLFunction f("getTriplanarBlend", "vec3 normal", "vec3");
	f.AddLine(GLSLLine("vec3 blending = abs(normal);", ""));
	f.AddLine(GLSLLine("blending = normalize(max(blending, 0.000001));", "Force weights to sum to 1.0"));
	f.AddLine(GLSLLine("float b = (blending.x + blending.y + blending.z);", ""));
	f.AddLine(GLSLLine("blending /= vec3(b);", ""));
	f.AddLine(GLSLLine("return blending;", ""));
	return f;
}

void ShaderTextureNode::OnEvaluate(GLSLFunction *function, GLSLLine *line)
{
	handler->AddFunction(GetTriplanarBlendFunction());
	function->AddLine(GLSLLine("vec3 " + VAR("tex") + ";", "Variable to store the result"));
	function->AddLine(GLSLLine("if(" + SDATA(4) + " == 1.0)", "If triplanar mapping is enabled"));
	function->AddLine(GLSLLine("{", ""));
	function->AddLine(GLSLLine(""
	                           "\tvec3 " + VAR("blending") + " = getTriplanarBlend(Normal);\n"
	                           "\t\tvec3 " + VAR("xaxis") + " = texture(_Textures, vec3(FragPos.yz * " + SDATA(0) + " + vec2(" + SDATA(1) + ", " + SDATA(2) + "), " + STR(zCoord) + ")).rgb;\n"
	                           "\t\tvec3 " + VAR("yaxis") + " = texture(_Textures, vec3(FragPos.xz * " + SDATA(0) + " + vec2(" + SDATA(1) + ", " + SDATA(2) + "), " + STR(zCoord) + ")).rgb;\n"
	                           "\t\tvec3 " + VAR("zaxis") + " = texture(_Textures, vec3(FragPos.xy * " + SDATA(0) + " + vec2(" + SDATA(1) + ", " + SDATA(2) + "), " + STR(zCoord) + ")).rgb;\n"
	                           "\t" + VAR("tex") + " = " + VAR("blending") + ".x * " + VAR("xaxis") + " + " + VAR("blending") + ".y * " + VAR("yaxis") + " + " + VAR("blending") + ".z * " + VAR("zaxis") + ";\n"
	                           , ""));
	function->AddLine(GLSLLine("}", ""));
	function->AddLine(GLSLLine("else", "If texture is not triplanar mapped just sample texture normally"));
	function->AddLine(GLSLLine("{", ""));
	function->AddLine(GLSLLine(""
	                           "\t" + VAR("tex") + " = texture(_Textures, vec3(TexCoord * " + SDATA(0) + " + vec2(" + SDATA(1) + ", " + SDATA(2) + "), " + STR(zCoord) + ")).rgb;\n"));
	function->AddLine(GLSLLine("}", ""));
	line->line = VAR("tex");
}

void ShaderTextureNode::Load(nlohmann::json data)
{
	if(texture != nullptr)
	{
		delete texture;
		texture = nullptr;
	}

	texture = new Texture2D(ProjectManager::Get()->GetResourcePath() + PATH_SEPARATOR + ProjectManager::Get()->GetAsset(data["texture"]));
	scale = data["scale"];
	offsetX = data["offsetX"];
	offsetY = data["offsetY"];
	rotation = data["rotation"];
	isTriplanar = data["isTriplanar"];
}

nlohmann::json ShaderTextureNode::Save()
{
	nlohmann::json data;
	data["type"] = "ShaderTexture";
	data["scale"] = scale;
	data["offsetX"] = offsetX;
	data["offsetY"] = offsetY;
	data["rotation"] = rotation;
	data["isTriplanar"] = isTriplanar;
	data["texture"] = ProjectManager::Get()->SaveTexture(texture);
	return data;
}

void ShaderTextureNode::UpdateShaders()
{
	sharedData->d0 = scale;
	sharedData->d1 = offsetX;
	sharedData->d2 = offsetY;
	sharedData->d3 = rotation;
	sharedData->d4 = isTriplanar ? 1.0f : 0.0f;
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
			{
				delete texture;
			}

			texture = new Texture2D(path);
			textureManager->UploadToGPU(zCoord);
		}
	}

	if(ImGui::BeginDragDropTarget())
	{
		if(const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TerraForge3D_Texture"))
		{
			std::string path = std::string((char*)payload->Data);
			if(path.size() > 3)
			{
				if(texture)
				{
					delete texture;
				}

				texture = new Texture2D(path);
				textureManager->UploadToGPU(zCoord);
			}
		}

		ImGui::EndDragDropTarget();
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

	if(ImGui::Checkbox("Triplanar Mapping", &isTriplanar))
	{
		sharedData->d4 = isTriplanar ? 1.0f : 0.0f;
	}

	ImGui::PopItemWidth();
}

ShaderTextureNode::ShaderTextureNode(GLSLHandler *handler, ShaderTextureManager *textureManager)
	:SNENode(handler), textureManager(textureManager)
{
	name = "Texture";
	headerColor = ImColor(SHADER_TEXTURE_NODE_COLOR);
	outputPins.push_back(new SNEPin(NodeEditorPinType::Output, SNEPinType::SNEPinType_Float3));
	texture = new Texture2D(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "textures" PATH_SEPARATOR "white.png");
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
