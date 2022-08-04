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

	static Terrain::Generator* ShowAddGeneratorButtons(ApplicationState* appState)
	{
		if (ImGui::Button("Dummy"))
			return new Terrain::Dummy_Generator(appState);
		return nullptr;
	}

	static SharedPtr<RendererAPI::Pipeline> renderPipeline[VIEWPORT_COUNT];

	// TEMP
	static std::string vss, fss;

	static SharedPtr<RendererAPI::SharedStorageBuffer> buff;
	static float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	Inspector::Inspector(ApplicationState* as)
	{
		this->appState = as;

		buff = RendererAPI::SharedStorageBuffer::Create();
		buff->SetSize(sizeof(color));
		buff->SetBinding(0);
		buff->Setup();
	}

	Inspector::~Inspector()
	{
		buff->Destroy();
	}

	void Inspector::OnStart()
	{
		this->appState->menus.mainMenu->GetManagerPTR()->Register("Windows/Inspector", [this](TerraForge3D::UI::MenuItem* context) {
			isVisible = context->GetToggleState();
			}, TerraForge3D::UI::MenuItemUse_Toggle)->RegisterTogglePTR(&isVisible);

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
					RendererAPI::ShaderVar("_Model", RendererAPI::ShaderDataType_Mat4)
					});
				
				// TEMP ------------------------------------------------------------------
				renderPipeline[i]->SetSharedStorageBuffer(buff.Get());
				
				renderPipeline[i]->shader->Compile();
				renderPipeline[i]->Setup();
			}

			this->terrain.manager = appState->terrain.manager.Get();

			// TEMP
#ifdef TF3D_VULKAN_BACKEND
			vulkanGPUName = Vulkan::ComputeDevice::Get()->physicalDevice.name;
#else // TF3D_OPENGL_BACKEND
			vulkanGPUName = "";
#endif
			openCLGPUName = "TBI";
			cpuName = "TBI";
	}

	void Inspector::OnShow()
	{
		if (ImGui::GetWindowHeight() < itemViewHeight)
			itemViewHeight = ImGui::GetWindowHeight() - separatorSliderWidth;

		float windowWidth = ImGui::GetWindowWidth();
		float windowHeight = ImGui::GetWindowHeight();

		ImGui::BeginChild("##InspectorItemDetails", ImVec2(windowWidth, itemViewHeight));
		Utils::ImGuiC::PushSubFont(appState->core.fonts["Header"].handle);
		ImGui::Text("Selected Item Details");
		Utils::ImGuiC::PopSubFont();

		if (isTerrainSelected)
		{
			ShowTerrainSettings();
		}
		else
		{
			ImGui::Text("Select an item from the list below so see relevant options");
		}

		ImGui::EndChild();

		ImGuiMouseCursor currentCursor = ImGui::GetMouseCursor();
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
		ImGui::Button("##InspectorSeperator", ImVec2(windowWidth, separatorSliderWidth));
		float currentMousePos = ImGui::GetMousePos().y;
		float mouseDelta = currentMousePos - prevMousePos;
		prevMousePos = currentMousePos;
		//if (fabs(mouseDelta) > 50.0f)
			//mouseDelta = 50.0f;
		if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			itemViewHeight += mouseDelta;
			if (itemViewHeight <= separatorSliderWidth)
				itemViewHeight = separatorSliderWidth;
			if (itemViewHeight > windowHeight - separatorSliderWidth)
				itemViewHeight = windowHeight - separatorSliderWidth;
		}
		ImGui::SetMouseCursor(currentCursor);

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

		static const char* MeshResolutionsStr[] = {
			"128x128",
			"256x256",
			"512x512",
			"1024x1024",
			"2048x2048",
			"4096x4096",
			"8192x8192",
		};

		// TEMP ----------------------------------------------------------------------
		if (ImGui::ColorEdit4("color", color))
			buff->SetData(color, sizeof(color), 0);

		Utils::ImGuiC::ComboBox("Resolution", &terrain.resolutionIndex, MeshResolutionsStr, TF3D_STATIC_ARRAY_SIZE(MeshResolutionsStr));
		
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("This is the resolution used to render the preview\n"
				"in the viewports. Though this has a maximumn\n"
				"resolution of 8192x8192 it has got nothing to do\n"
				"with baking. You can bake in any resolution at all.");
		}
		ImGui::DragFloat("Scale", &terrain.scale, 0.01f, 1.0f);


		if (terrain.manager->resolution != static_cast<uint32_t>(pow(2, 7 + terrain.resolutionIndex))
		|| terrain.manager->scale != terrain.scale)
		{
			ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Settings not yet applied");
		}

		if (ImGui::Button("Apply Changes"))
		{
			terrain.resolution = static_cast<uint32_t>(pow(2, 7 + terrain.resolutionIndex));
			JobSystem::Job* resizeJob = new JobSystem::Job("Mesh Reisize Job");
			resizeJob->description = "Resize the mesh";
			resizeJob->excutionModel = JobSystem::JobExecutionModel_Async;
			resizeJob->onRun = [this](JobSystem::Job* context) -> bool {
				terrain.manager->Resize(terrain.resolution, terrain.scale, &context->progress);
				return true;
			};
			resizeJob->onComplete = [this](JobSystem::Job* context)-> void {
				terrain.manager->mesh->UploadToGPU(); // Temporary
			};
			appState->modals.manager->LoadingBox("Resizing Mesh", &resizeJob->progress);
			appState->jobs.manager->AddJob(resizeJob);
		}

		Utils::ImGuiC::ComboBox("Generator Device", &generators.device, Terrain::ProcessorDeviceStr, TF3D_STATIC_ARRAY_SIZE(Terrain::ProcessorDeviceStr));
		
		switch (generators.device)
		{
		case Terrain::ProcessorDevice_CPU:
			ImGui::Text("Processor Device Name : %s", cpuName.data());
			break;
#ifdef TF3D_VULKAN_BACKEND
		case ProcessorDevice_Vulkan:
			ImGui::Text("Processor Device Name : %s", vulkanGPUName.data());
			break;
#endif
		case Terrain::ProcessorDevice_OpenCL:
			ImGui::Text("Processor Device Name : %s", openCLGPUName.data());
			break;
		default:			
			ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Unknown Processor Device Selected");
		break;
		}

		Utils::ImGuiC::PushSubFont(appState->core.fonts["SemiHeader"].handle);
		ImGui::Text("Generators");
		Utils::ImGuiC::PopSubFont();

		ImGui::Separator();

		terrain.manager->ShowGeneratorSettings();


		if (ImGui::Button("Add"))
		{
			ImGui::OpenPopup("##InscpectorAddGenerator");
		}

		Terrain::Generator* temp = nullptr;
		if (ImGui::BeginPopup("##InscpectorAddGenerator"))
		{
			ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth());
			temp = ShowAddGeneratorButtons(appState);
			if (temp)
			{
				terrain.manager->AddGenerator(temp);
				this->GetSubEditorManager()->AddEditor(temp->editor);
				temp = nullptr;
				ImGui::CloseCurrentPopup();
			}
			ImGui::PopItemWidth();
			ImGui::EndPopup();
		}		
	}

	void Inspector::OnUpdate()
	{
	}

	void Inspector::OnEnd()
	{
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
		appState->renderer->DrawMesh(appState->terrain.manager->mesh.Get(), 484);
		return true;
	}
}