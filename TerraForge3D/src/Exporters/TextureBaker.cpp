#include "Exporters/TextureBaker.h"
#include "Data/ApplicationState.h"

#include "imgui/imgui.h"

TextureBaker::TextureBaker(ApplicationState *appState)
    :appState(appState)
{
    previewFrameBuffer = new FrameBuffer(512, 512);
}

TextureBaker::~TextureBaker()
{
    delete previewFrameBuffer;
}

void TextureBaker::ShowSettings(bool *pOpen)
{
    ImGui::Begin("Texture Baker", pOpen);
    
    if(ImGui::CollapsingHeader("Preview"))
    {
        ImGui::Image((ImTextureID)previewFrameBuffer->GetColorTexture(), ImVec2(256, 256));
    }

	if (ImGui::BeginCombo("Texture Slot##combo", textueBakerSlots[currentTextureSlot]))
    {
        for (int i = 0; i < IM_ARRAYSIZE(textueBakerSlots); i++)
        {
            bool is_selected = (currentTextureSlot == i);
            if (ImGui::Selectable(textueBakerSlots[i], is_selected))
            {
                currentTextureSlot = i;
            }

            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Checkbox("Use Tiled Export", &useTiledExport);
    
    if(useTiledExport)
    {
        ImGui::DragInt("Tile Resolution", &tileResolution);
        ImGui::DragInt("Tile Count", &tileCount);

        ImGui::InputInt("Tile X", &tileX);
        ImGui::InputInt("Tile Y", &tileY);
    }
    else
    {
        ImGui::DragInt("Bake Resolution", &tileResolution);
    }

    if(currentTextureSlot == 0)
    {
        if(ImGui::Button("Calculate Heigt Map Min Max"))
        {
            while(appState->states.remeshing);
            CalculateHeightMapMinMax();
        }
        ImGui::DragFloat2("Height Map Min Max", heightMapMinMax, 0.01f);
    }

    if(ImGui::Button("Bake"))
    {
        Bake();
    }

    ImGui::End();

    Update();
}

void TextureBaker::CalculateHeightMapMinMax()
{
    float min = 10e10;
    float max = -10e10;

    Vert* vert = appState->models.coreTerrain->mesh->vert;
    int vertCount = appState->models.coreTerrain->mesh->vertexCount;

    for(int i = 0; i < vertCount; i++)
    {
        if(vert[i].position.y < min)
        {
            min = vert[i].position.y;
        }
        if(vert[i].position.y > max)
        {
            max = vert[i].position.y;
        }
    }

    heightMapMinMax[0] = min;
    heightMapMinMax[1] = max;
    
}

void TextureBaker::Bake()
{
    if(bakeFrameBuffer != nullptr)
    {
        delete bakeFrameBuffer;
    }
    
    if(useTiledExport)
    {
        std::string path = ShowSaveFileDialog(".png");
        if(path.size() <= 3)
            return;
        std::string nameBase = "";
        if(path.find_last_of(".png") == std::string::npos)
            nameBase = path;
        else
            nameBase = path.substr(0, path.find_last_of(".png"));
        
        if(tileCount < 0)
            tileCount = -1 * tileCount;
        if(tileCount == 0)
            tileCount = 2;
        if(tileCount % 2 != 0)
            tileCount += 1;
        
        
        bakeFrameBuffer = new FrameBuffer(tileResolution, tileResolution);

        std::cout << "Starting to render tiles." << std::endl;
        for(int i = -1 * tileCount + 1 ; i <=  tileCount - 1 ; i+= 2)
        {
            for(int j = -1 * tileCount + 1 ; j <=  tileCount - 1 ; j+= 2)
            {
                tileY = i;
                tileX = j;
                std::cout << "Rendering Tile : " << (i + tileCount) << ", " << (j + tileCount) << "\r";
                bakeFrameBuffer->Begin();    
                glViewport(0, 0, tileResolution, tileResolution);
		        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		        Render(false);
        		ExportTexture(bakeFrameBuffer->GetRendererID(), nameBase + "_" + std::to_string((i + tileCount)/2) + "_" + std::to_string((j + tileCount)/2) + ".png", tileResolution, tileResolution);
                std::cout << "Rendered Tile : " << (i + tileCount) / 2 << ", " << (j + tileCount) / 2 << " to " << (nameBase + "_" + std::to_string((i + tileCount)/2) + "_" + std::to_string((j + tileCount)/2) + ".png") << std::endl;
            }

        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
		delete bakeFrameBuffer;
		bakeFrameBuffer = nullptr;
    }
    else
    {
        std::string path = ShowSaveFileDialog(".png");
        if(path.size() <= 3)
            return;
        bakeFrameBuffer = new FrameBuffer(tileResolution, tileResolution);
        bakeFrameBuffer->Begin();    
		glViewport(0, 0, tileResolution, tileResolution);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		Render(false);
		ExportTexture(bakeFrameBuffer->GetRendererID(), path, tileResolution, tileResolution);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		delete bakeFrameBuffer;
		bakeFrameBuffer = nullptr;
    }
}

void TextureBaker::Update()
{
	previewFrameBuffer->Begin();
	glViewport(0, 0, previewFrameBuffer->GetWidth(), previewFrameBuffer->GetHeight());
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Render(true);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TextureBaker::Render(bool isPreview)
{
    textureBakerCamera = appState->cameras.main;
	textureBakerCamera.aspect = 1.0f;
	textureBakerCamera.position[0] = textureBakerCamera.position[1] = textureBakerCamera.position[2] = 0.0f;
	textureBakerCamera.position[2] = 1.0f;
	textureBakerCamera.rotation[0] = textureBakerCamera.rotation[1] = textureBakerCamera.rotation[2] = 0.0f;
	textureBakerCamera.perspective = false;
    textureBakerCamera.UpdateCamera();

	float tmp[3];

	if(isPreview)
	{
		tmp[0] = tmp[1] = 512.0f;
	}
	else
	{
		tmp[0] = tileResolution;
		tmp[1] = tileResolution;
	}

    tmp[2] = 1.0f;

    Shader* shader = appState->shaders.terrain;
    shader->Bind();
    shader->SetMPV(textureBakerCamera.pv);
	shader->SetUniformMat4("_Model", appState->models.coreTerrain->modelMatrix);
	shader->SetLightCol(appState->lightManager->color);
	shader->SetLightPos(appState->lightManager->position);
	shader->SetUniform3f("_Resolution", tmp);
	shader->SetUniform3f("_CameraPos", appState->cameras.main.position);
	shader->SetUniform3f("_HMapMinMax", heightMapMinMax);
	shader->SetUniformf("_SeaLevel", appState->seaManager->level);
	shader->SetUniformf("_Scale", appState->globals.scale);
	shader->SetUniformf("_TextureBake", 1.0f);
	shader->SetUniformf("_CameraNear", appState->cameras.main.cNear);
	shader->SetUniformf("_CameraFar", appState->cameras.main.cFar);
	shader->SetUniformf("_Slot", static_cast<float>(currentTextureSlot));
    shader->SetUniformf("_Tiled", useTiledExport ? 1.0f : 0.0f);
    shader->SetUniformf("_NumTiles", static_cast<float>(tileCount));
    shader->SetUniformf("_TileX", static_cast<float>(-1 * tileX));
    shader->SetUniformf("_TileY", static_cast<float>(-1 * tileY));
	appState->shadingManager->UpdateShaders();
	appState->models.coreTerrain->Render();    
}

nlohmann::json TextureBaker::Save()
{
    nlohmann::json data;
    data["type"] = "TextureBaker";
    data["useTiledExport"] = useTiledExport;
    data["tileResolution"] = tileResolution;
    data["tileCount"] = tileCount;
    // data["autoCalculateHeightMapMinMax"] = autoCalculateHeightMapMinMax;
    data["heightMapMinMax.x"] = heightMapMinMax[0];
    data["heightMapMinMax.y"] = heightMapMinMax[1];
    data["currentTextureSlot"] = currentTextureSlot;

    return data;
}

void TextureBaker::Load(nlohmann::json data)
{
    useTiledExport = data["useTiledExport"];
    tileResolution = data["tileResolution"];
    tileCount = data["tileCount"];
    // autoCalculateHeightMapMinMax = data["autoCalculateHeightMapMinMax"];
    heightMapMinMax[0] = data["heightMapMinMax.x"];
    heightMapMinMax[1] = data["heightMapMinMax.y"];
    currentTextureSlot = data["currentTextureSlot"];
}