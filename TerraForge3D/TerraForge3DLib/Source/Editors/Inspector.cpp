#include "Editors/Inspector.hpp"

#include "TerraForge3D.hpp"


#ifdef TF3D_VULKAN_BACKEND
#include "Base/Vulkan/ComputeDevice.hpp"
#else // TF3D_OPENGL_BACKEND
#include "Base/Renderer/Context.hpp"
#include "Base/OpenGL/Context.hpp"
#endif

#include "imgui/imgui_internal.h"


namespace TerraForge3D
{

	static SharedPtr<RendererAPI::Pipeline> renderPipeline[VIEWPORT_COUNT];

	// TEMP
	static std::string vss, fss;
	static TerrainPointData ptd[1024 * 1024];

	static void GenerateTerrain(int res)
	{
		for(int i = 0 ; i < res ; i++)
		{
			for(int j = 0 ; j < res ; j++)
			{
				float x = (float)j / res;
				float y = (float)i / res;
				ptd[i * res + j].a.x = fabs(sin(x * 8) + cos(y * 8));
			}
		}
	}

	Inspector::Inspector(ApplicationState* as)
		:UI::Editor("Inspector")
	{
		this->appState = as;
	}

	Inspector::~Inspector()
	{
	}

	void Inspector::OnStart()
	{
		this->appState->menus.mainMenu->GetManagerPTR()->Register("Windows/Inspector", [this](TerraForge3D::UI::MenuItem* context) {
			isVisible = context->GetToggleState();
			}, TerraForge3D::UI::MenuItemUse_Toggle)->RegisterTogglePTR(&isVisible);

		terrain.mesh = new Mesh();
		terrain.mesh->Plane(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		terrain.mesh->RecalculateMatices();
		terrain.mesh->RecalculateNormals();
		terrain.mesh->UploadToGPU();
		terrain.mesh->Clear();

		terrain.dataBuffer = RendererAPI::SharedStorageBuffer::Create();
		terrain.dataBuffer->UseForGraphics();
		terrain.dataBuffer->SetSize(sizeof(TerrainPointData) * 256 * 256);
		terrain.dataBuffer->SetBinding(0);
		terrain.dataBuffer->Setup();


		vss = Utils::ReadTextFile(appState->appResourcePaths.shaderIncludeDir + PATH_SEPERATOR "Vert.glsl");
		fss = Utils::ReadTextFile(appState->appResourcePaths.shaderIncludeDir + PATH_SEPERATOR "Frag.glsl");

		for (int i = 0; i < VIEWPORT_COUNT; i++)
		{
			renderPipeline[i] = RendererAPI::Pipeline::Create();
			renderPipeline[i]->shader->SetIncludeDir(appState->appResourcePaths.shaderIncludeDir);
			renderPipeline[i]->shader->SetSource(vss, RendererAPI::ShaderStage_Vertex);
			renderPipeline[i]->shader->SetSource(fss, RendererAPI::ShaderStage_Fragment);
			renderPipeline[i]->shader->SetUniformsLayout({
			RendererAPI::ShaderVar("_Engine", RendererAPI::ShaderDataType_IVec4),
				RendererAPI::ShaderVar("_PV", RendererAPI::ShaderDataType_Mat4),
				RendererAPI::ShaderVar("_Model", RendererAPI::ShaderDataType_Mat4),
				RendererAPI::ShaderVar("_Resolution", RendererAPI::ShaderDataType_Vec4)
			});
			
			renderPipeline[i]->AddSharedStorageBuffer(this->terrain.dataBuffer.Get());
				
			renderPipeline[i]->shader->Compile();
			renderPipeline[i]->Setup();
		}

		this->generatorSettings = new GeneratorSettings(this->appState);
		this->generatorSettings->SetVisible(false);
		this->GetSubEditorManager()->AddEditor(this->generatorSettings);			
	}

	void Inspector::OnShow()
	{
		itemViewHeight = std::max(itemViewHeight, (ImGui::GetWindowHeight() - separatorSliderWidth) / 2.0f);

		float windowWidth = ImGui::GetWindowWidth();
		float windowHeight = ImGui::GetWindowHeight();

		ImGui::BeginChild("##InspectorItemDetails", ImVec2(windowWidth, itemViewHeight));
		Utils::ImGuiC::PushSubFont(appState->core.fonts["Header"].handle);
		ImGui::Text("Selected Item Details");
		Utils::ImGuiC::PopSubFont();

		if (isTerrainSelected)
			ShowTerrainSettings();
		else
			ImGui::Text("Select an item from the list below so see relevant options");

		ImGui::EndChild();

		ImGuiMouseCursor currentCursor = ImGui::GetMouseCursor();
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
		ImGui::Button("##InspectorSeperator", ImVec2(windowWidth, separatorSliderWidth));
		float currentMousePos = ImGui::GetMousePos().y;
		float mouseDelta = currentMousePos - prevMousePos;
		prevMousePos = currentMousePos;
		if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			itemViewHeight += mouseDelta;
			if (itemViewHeight <= separatorSliderWidth)
				itemViewHeight = separatorSliderWidth;
			if (itemViewHeight > windowHeight - separatorSliderWidth)
				itemViewHeight = windowHeight - separatorSliderWidth;
		}		ImGui::SetMouseCursor(currentCursor);
		ImGui::BeginChild("##InspectorList", ImVec2(windowWidth, windowHeight - itemViewHeight - separatorSliderWidth - 30));
		Utils::ImGuiC::PushSubFont(appState->core.fonts["Header"].handle);
		ImGui::Text("Objects");
		Utils::ImGuiC::PopSubFont();
		ImGui::Selectable("Terrain", &isTerrainSelected, 0, ImVec2(windowWidth, 20.0f));

		ImGui::EndChild();
	}


	void Inspector::ShowTerrainSettings()
	{
		float windowWidth = ImGui::GetWindowWidth();
		
		Utils::ImGuiC::PushSubFont(appState->core.fonts["SemiHeader"].handle);
		ImGui::Text("Preview Settings");
		Utils::ImGuiC::PopSubFont();

		if(ImGui::Button("Reload Shaders"))
		{
			std::string vs = Utils::ReadTextFile(appState->appResourcePaths.shaderIncludeDir + PATH_SEPERATOR "Vert.glsl");
			renderPipeline[0]->shader->SetSource(vs, RendererAPI::ShaderStage_Vertex);
			renderPipeline[0]->shader->Compile();
			renderPipeline[0]->ForceRequireRebuild();
		}

		if(ImGui::Button("Generator Settings"))
			this->generatorSettings->SetVisible(true);
	}

	void Inspector::OnUpdate()
	{
		static int oldResolution = 256; 		
		if(appState->terrain.previewResolution != oldResolution)
		{
			oldResolution = appState->terrain.previewResolution;
			terrain.resolutionData[0] = (float)oldResolution;
			terrain.resolutionData[1] = (float)oldResolution;
			this->isRemeshing = true;
			JobSystem::Job* job = new JobSystem::Job("Resize Terrain");
			job->excutionModel = JobSystem::JobExecutionModel_Async;
			job->onRun = [this] (JobSystem::Job* context) -> bool {
				appState->modals.manager->LoadingBox("Generating Mesh", &context->progress);
				terrain.mesh->Plane(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), oldResolution, 1.0f, &context->progress);
				terrain.mesh->RecalculateMatices();
				terrain.mesh->RecalculateNormals();
				GenerateTerrain(oldResolution);
				return true;
			};
			job->onComplete = [this] (JobSystem::Job* context) -> bool {
				terrain.mesh->UploadToGPU();
				terrain.mesh->Clear();
				terrain.dataBuffer->Destroy();
				terrain.dataBuffer->UseForGraphics();
				terrain.dataBuffer->SetSize(sizeof(TerrainPointData) * oldResolution * oldResolution);
				terrain.dataBuffer->SetBinding(0);
				terrain.dataBuffer->Setup();
				terrain.dataBuffer->SetData(ptd, terrain.dataBuffer->GetSize(), 0);
				this->isRemeshing = false;
				return true;
			};
			appState->jobs.manager->AddJob(job);
		}
	}

	void Inspector::UpdateTerrainData(TerrainGeneratorState* state)
	{
		if(state->outputResolution[0] * state->outputResolution[0] * sizeof(TerrainPointData) == terrain.dataBuffer->GetSize())
			terrain.dataBuffer->SetData(state->data, terrain.dataBuffer->GetSize(), 0);
	}


	void Inspector::OnEnd()
	{
		terrain.mesh->Clear(true);
		terrain.dataBuffer->Destroy();
		for (int i = 0; i < VIEWPORT_COUNT; i++)
			renderPipeline[i]->Destory();
	}

	bool Inspector::OnContextMenu()
	{
		return false;
	}

	bool Inspector::RenderItems(Viewport* viewport)
	{
		appState->renderer->SetCamera(viewport->GetCamera().Get());
		appState->renderer->SetClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		appState->renderer->BindFramebuffer(viewport->GetFramebuffer().Get());
		appState->renderer->ClearFrame();
		appState->renderer->BindPipeline(renderPipeline[viewport->GetNumber()].Get());
		appState->renderer->UploadUnifrom("_Resolution", terrain.resolutionData); 
		appState->renderer->DrawMesh(terrain.mesh.Get(), 484);
		return true;
	}
}