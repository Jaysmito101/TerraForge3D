#include "Sea/SeaManager.h"

#include "Utils/Utils.h"
#include "Base/Base.h"
#include "Data/ProjectData.h"

SeaManager::SeaManager()
{
	model = new Model("Sea");
	model->SetupMeshOnGPU();
	model->mesh->GeneratePlane(256, 120);
	model->mesh->RecalculateNormals();
	model->UploadToGPU();
	bool tmp = false;
	enabled = false;
	color[0] = 0;
	color[1] = 0;
	color[2] = 0.7f;
	color[3] = 0;
	scale = 100;
	std::string vertexShaderSource = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\water\\vert.glsl", &tmp);
	std::string fragmentShaderSource = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\water\\frag.glsl", &tmp);
	std::string geometryShaderSource = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\shaders\\water\\geom.glsl", &tmp);
	shader = new Shader(vertexShaderSource, fragmentShaderSource, geometryShaderSource);
	dudvMap = new Texture2D(GetExecutableDir() + "\\Data\\textures\\water_dudv.png");
	normalMap = new Texture2D(GetExecutableDir() + "\\Data\\textures\\water_normal.png");
}

SeaManager::~SeaManager()
{
	if (model)
	{
		delete model;
	}

	if (shader)
	{
		delete shader;
	}

	if (dudvMap)
	{
		delete dudvMap;
	}

	if (normalMap)
	{
		delete normalMap;
	}
}

void SeaManager::Load(nlohmann::json data)
{
	alpha = data["alpha"];
	distrotionStrength = data["distrotionStrength"];
	distrotionScale = data["distrotionScale"];
	reflectivity = data["reflectivity"];
	level = data["level"];
	waveSpeed = data["waveSpeed"];
	enabled = data["enabled"];
	color[0] = data["color"]["r"];
	color[1] = data["color"]["g"];
	color[2] = data["color"]["b"];

	if (dudvMap)
	{
		delete dudvMap;
	}

	dudvMap = new Texture2D(GetProjectResourcePath() + "\\" + GetProjectAsset(data["dudvMap"]));

	if (normalMap)
	{
		delete normalMap;
	}

	normalMap = new Texture2D(GetProjectResourcePath() + "\\" + GetProjectAsset(data["normalMap"]));
}

nlohmann::json SeaManager::Save()
{
	nlohmann::json data;
	data["type"] = "Sea Settings";
	data["alpha"] = alpha;
	data["distrotionStrength"] = distrotionStrength;
	data["distrotionScale"] = distrotionScale;
	data["reflectivity"] = reflectivity;
	data["level"] = level;
	data["waveSpeed"] = waveSpeed;
	data["enabled"] = enabled;
	nlohmann::json jcolor;
	jcolor["r"] = color[0];
	jcolor["g"] = color[1];
	jcolor["b"] = color[2];
	data["color"] = jcolor;
	data["dudvMap"] = SaveProjectTexture(dudvMap);
	data["normalMap"] = SaveProjectTexture(normalMap);
	return data;
}

void SeaManager::Render(Camera &camera, LightManager *lights, void *reflectionTexture, float time)
{
	shader->Bind();
	shader->SetTime(&time);
	shader->SetUniformf("_SeaAlpha", alpha);
	shader->SetUniformf("_SeaDistScale", distrotionScale);
	shader->SetUniformf("_SeaDistStrength", distrotionStrength);
	shader->SetUniformf("_SeaReflectivity", reflectivity);
	shader->SetUniformf("_SeaLevel", level);
	shader->SetUniformf("_SeaWaveSpeed", waveSpeed);
	glActiveTexture(7);
	glBindTexture(GL_TEXTURE_2D, (GLuint)reflectionTexture);
	shader->SetUniformi("_ReflectionTexture", 7);

	if (dudvMap)
	{
		dudvMap->Bind(8);
	}

	shader->SetUniformi("_DuDvMap", 8);

	if (normalMap)
	{
		normalMap->Bind(9);
	}

	shader->SetUniformi("_NormalMap", 9);
	model->position.y = level;
	model->scale.x = scale;
	model->scale.z = scale;
	model->Update();
	shader->SetUniform3f("_SeaColor", color);
	glm::mat4 mpv = camera.pv * model->modelMatrix;
	shader->SetMPV(mpv);
	shader->SetLightCol(lights->color);
	shader->SetLightPos(lights->position);
	model->Render();
}

void SeaManager::ShowSettings(bool *pOpen)
{
	if (*pOpen)
	{
		ImGui::Begin("Sea Setings", pOpen);
		ImGui::Checkbox("Sea Enabled", &enabled);
		ImGui::DragFloat("Sea Level", &level, 0.1f);
		ImGui::Text("Sea Color");
		ImGui::ColorEdit3("##seaColor", color);
		ImGui::DragFloat("Alpha", &alpha, 0.01f, 0, 1);
		ImGui::DragFloat("Scale", &scale, 0.1f);
		ImGui::DragFloat("Reflectvity", &reflectivity, 0.01f, 0, 1);
		ImGui::DragFloat("Distortion Strength", &distrotionStrength, 0.001f, 0, 1);
		ImGui::DragFloat("Distortion Scale", &distrotionScale, 0.001f, 0, 1);
		ImGui::DragFloat("Wave Speed", &waveSpeed, 0.01f, 0, 1);
		ImGui::Text("DuDv Map");
		ImGui::SameLine();

		if (ImGui::ImageButton((ImTextureID)dudvMap->GetRendererID(), ImVec2(50, 50)))
		{
			LoadFileIntoTexture(dudvMap);
		}

		ImGui::Text("Normal Map");
		ImGui::SameLine();

		if (ImGui::ImageButton((ImTextureID)normalMap->GetRendererID(), ImVec2(50, 50)))
		{
			LoadFileIntoTexture(normalMap);
		}

		ImGui::End();
	}
}
