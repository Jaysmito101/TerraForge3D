#include "TerraForge3D.hpp"
#include "EntryPoint.hpp"

#ifdef TF3D_WINDOWS
// For the windows API
#undef MessageBox
#undef AddJob
#endif

class MyEditor : public TerraForge3D::UI::Editor
{
public:
	MyEditor(std::string name, TerraForge3D::ApplicationState* appState)
		: TerraForge3D::UI::Editor(name), appState(appState)
	{
	}

	void OnUpdate() override
	{
		counter += 0.001f;
		if (counter >= 1.5f)
			counter = 0.0f;
	}

	void OnShow() override
	{
		ImGui::Text("Counter : %f", counter);
		static char styleName[4096] = {};


		ImGui::InputTextWithHint("Style Name", "Enter Style Name", styleName, 4096);
		if (ImGui::Button("Load Style"))
		{
			style.LoadFromFile(appState->appResourcePaths.stylesDir + PATH_SEPERATOR + std::string(styleName) + ".json");
			style.Apply();
		}
		ImGui::NewLine();

		static char message[4096];
		ImGui::InputTextMultiline("Message", message, 4096);
		static int mtype;
		ImGui::InputInt("Type", &mtype, 1, 100);
		if (ImGui::Button("Show Message Box"))
		{
			appState->modals.manager->MessageBox("Message", message, static_cast<TerraForge3D::UI::MessageType>(mtype));
		}

		ImGui::NewLine();

		if (ImGui::Button("Show Loading Box"))
		{
			appState->modals.manager->LoadingBox("Loading", &counter, [this](float progress)->bool {
				counter = 0.0f;
				return true;
				});
		}

		ImGui::NewLine();

		static std::string lastOpenedFile = "";
		
		if(lastOpenedFile.size() > 0)
			ImGui::Text(lastOpenedFile.data());

		if (ImGui::Button("Show File Dialog"))
		{
			TerraForge3D::UI::FileDialogInfo fdi;
			fdi.onSelect = [this](TerraForge3D::UI::FileDialogInfo* fd) -> void
			{
				lastOpenedFile = fd->selectedFilePath;
			};
			appState->modals.manager->FileDialog("Open File", fdi);
		}

		static float jobSleepDuration = 1.0f;
		ImGui::DragFloat("Job Sleep Duration", &jobSleepDuration, 0.01f, 0.0001f);
		if (ImGui::Button("Add Dummy Job(AMT)"))
		{
			TerraForge3D::JobSystem::Job* job = new TerraForge3D::JobSystem::Job("Temp Job");
			job->excutionModel = TerraForge3D::JobSystem::JobExecutionModel_AsyncOnMainThread;
			job->onRun = [](TerraForge3D::JobSystem::Job* j) -> bool
			{
				std::this_thread::sleep_for(std::chrono::duration<float>(jobSleepDuration));
				TF3D_LOG("Job Done AMT");
				return true;
			};
			appState->jobs.manager->AddJob(job);
		}

		if (ImGui::Button("Add Dummy Job(A)"))
		{
			TerraForge3D::JobSystem::Job* job = new TerraForge3D::JobSystem::Job("Temp Job");
			job->excutionModel = TerraForge3D::JobSystem::JobExecutionModel_Async;
			job->onRun = [](TerraForge3D::JobSystem::Job* j) -> bool
			{
				std::this_thread::sleep_for(std::chrono::duration<float>(jobSleepDuration));
				TF3D_LOG("Job Done A");
				return true;
			};
			appState->jobs.manager->AddJob(job);
		}

		if (ImGui::Button("Exit"))
			appState->core.app->Close();

	}

	void OnStart() override
	{
		TF3D_LOG("On Start");
		appState->menus.mainMenu->GetManagerPTR()->Register("Windows/Style Opener", [this](TerraForge3D::UI::MenuItem* context) {
			isVisible = (context->GetToggleState());
			}, TerraForge3D::UI::MenuItemUse_Toggle)->RegisterTogglePTR(&isVisible);

		style.LoadFromFile(appState->appResourcePaths.stylesDir + PATH_SEPERATOR + "Maya.json");
		style.Apply();
	}

	void OnEnd() override
	{
		TF3D_LOG("On End");
	}

private:
	TerraForge3D::ApplicationState* appState = nullptr;
	TerraForge3D::UI::Style style;
	float counter = 0.0f;
};

#include <glad/glad.h>

std::string vss = R"(
#version 430 core

#include "Animations/SinY.glsl"

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 texCoord;
layout (location = 2) in vec2 normal;
layout (location = 3) in vec4 extra;

uniform mat4 _Model;
uniform mat4 _PV;

void main()
{
	gl_Position = _PV * vec4(sin_y(position.xyz), 1.0f);
}

)";

static std::string fss = R"(
#version 430 core
out vec4 FragColor;

void main()
{
	FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}
)";

#include "Base/OpenGL/Shader.hpp"

struct UBO
{
	glm::mat4 _PV = glm::mat4(1.0f);
	glm::mat4 _Model = glm::mat4(1.0f);
	glm::vec4 _Extra = glm::vec4(0.0f);
};


namespace TerraForge3D
{
	class MainApplication : public Application
	{
	public:
		virtual void OnPreload()
		{
			appState = ApplicationState::Create();
			SetLogFilePath(appState->appResourcePaths.currentLogFilePath);
			SetApplicationName(appState->meta.name);
			SetWindowConfigPath(appState->appResourcePaths.windowConfigPath);
			SetFontConfig(Utils::ReadTextFile(appState->appResourcePaths.fontsConfigPath), appState->appResourcePaths.fontsDir);
		}

		virtual void OnStart() override
		{
			TF3D_LOG("Started Application!");
			exitcb = GetInputEventManager()->RegisterCallback([&](InputEventParams* params) -> bool {
				TF3D_LOG_INFO("Shutting down application.");
				Close();
				return true;
				}, { InputEventType_WindowClose });
			
			Utils::ImGuiC::SetIconFont(fonts["MaterialIcons"].handle);

			// Object Creations
			appState->core.app = this;
			appState->core.window = GetWindow();
			appState->menus.mainMenu = new MainMenu(appState);
			appState->preferences = new Preferences(appState);
			appState->project.manager = new ProjectManager(appState);
			appState->editors.manager = new UI::EditorManager("Primary Editor Manager");
			appState->modals.manager = new UI::ModalManager(appState);
			appState->jobs.manager = new JobSystem::JobSystem(appState);

			editor = new MyEditor("Style Opener", appState);
			appState->editors.manager->AddEditor(editor);
			appState->editors.startUpScreen = reinterpret_cast<StartUpScreen*>(appState->editors.manager->AddEditor(new StartUpScreen(appState)));
			appState->editors.manager->AddEditor(new JobManager(appState));
			appState->editors.manager->AddEditor(appState->preferences->GetEditor());


			fbo = RendererAPI::FrameBuffer::Create();
			fbo->SetSize(512, 512);
			fbo->SetColorAttachmentCount(1);
			fbo->SetDepthAttachment(true);
			fbo->Setup();

			pipeline = RendererAPI::Pipeline::Create();
			
			pipeline->shader->SetIncludeDir(appState->appResourcePaths.shaderIncludeDir);
			pipeline->shader->SetSource(vss, RendererAPI::ShaderStage_Vertex);
			pipeline->shader->SetSource(fss, RendererAPI::ShaderStage_Fragment);
			pipeline->shader->SetUBOLayout({
				RendererAPI::UBOEntry("_U1", RendererAPI::ShaderDataType_Vec4),
				RendererAPI::UBOEntry("_U2", RendererAPI::ShaderDataType_Vec4),
				RendererAPI::UBOEntry("_U3", RendererAPI::ShaderDataType_Vec4),
				RendererAPI::UBOEntry("_U4", RendererAPI::ShaderDataType_Vec4)
				});
			pipeline->shader->Compile();
			pipeline->Setup();

			mesh = new Mesh("DemoTriangle");
			mesh->Clear();
			float A[3] = {  0.5f,  0.5f, 0.0f };
			float B[3] = {  0.5f, -0.5f, 0.0f };
			float C[3] = { -0.5f,  0.5f, 0.0f };
			mesh->Triangle(A, B, C);
			mesh->UploadToGPU();

			camera = new RendererAPI::Camera();
			camera->SetPerspective(90.0f, 1.0f, 0.01f, 1000.0f);
			camera->SetPosition(0.0, 0.0, -1.f);
			camera->SetRotation(0.0, 0.0, 0.0);

			appState->core.fonts = fonts;

			appState->renderer = appState->core.app->renderer;
		}

		virtual void OnUpdate() override
		{
			appState->editors.manager->Update();
			appState->modals.manager->Update();
			appState->project.manager->Update();
			appState->jobs.manager->Update();

			
			// TEMP

			appState->renderer->BindFramebuffer(fbo);
			appState->renderer->SetClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
			appState->renderer->ClearFrame();
			appState->renderer->BindPipeline(pipeline);
			appState->renderer->BindCamera(camera);
			appState->renderer->DrawMesh(mesh);
			// TEMP

			appState->renderer->Flush();


		}

		virtual void OnImGuiRender() override
		{
			dockspace.Begin();

			ImGui::PushFont(fonts["Default"].handle);

			appState->modals.manager->Show();
			if (!appState->modals.manager->IsActive())
			{
				appState->menus.mainMenu->Show();
				appState->editors.manager->Show();

				// ImGui::Begin("sadsa");
				// ImGui::ShowStyleEditor();
				// ImGui::End();
			}


			ImGui::Begin("Viewport");
			ImGui::Text("Begin");
			ImGui::Image(fbo->GetColorAttachmentImGuiID(0), ImVec2(256, 256));
			ImGui::Text("End");
			ImGui::ColorEdit4("ClearColor", clearColor);
			if (ImGui::DragFloat3("Camera Position", camPos, 0.1f) || ImGui::DragFloat3("Camera Rotation", camRot, 0.1f))
			{
				camera->SetPosition(camPos[0], camPos[1], camPos[2]);
				camera->SetRotation(camRot[0], camRot[1], camRot[2]);
				camera->RecalculateMatrices();
			}

			if (ImGui::Button("ReCompile Shaders"))
			{
				pipeline->shader->SetSource(vss, RendererAPI::ShaderStage_Vertex);
				pipeline->shader->SetSource(fss, RendererAPI::ShaderStage_Fragment);
				pipeline->shader->Compile();
				pipeline->Setup();
			}

			ImGui::End();
			ImGui::PopFont();

			dockspace.End();
		}

		virtual void OnEnd() override
		{
			delete camera;
			delete mesh;
			delete pipeline;
			delete fbo;
			
			GetInputEventManager()->DeregisterCallback(exitcb);
			
			// Object Destructions
			TF3D_SAFE_DELETE(appState->menus.mainMenu);
			TF3D_SAFE_DELETE(appState->editors.manager);
			TF3D_SAFE_DELETE(appState->modals.manager);
			TF3D_SAFE_DELETE(appState->project.manager);
			TF3D_SAFE_DELETE(appState->jobs.manager);
			TF3D_SAFE_DELETE(appState->preferences);


			ApplicationState::Destory();

			TF3D_LOG("Shutdown!");
		}

	private:
		uint32_t exitcb;
		float pos[2] = {0.0f};
		float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f};
		float camPos[3] = {0.0f, 0.0f, -1.0f};
		float camRot[3] = { 0.0f, 0.0f, 0.0f };

		ApplicationState* appState = nullptr;
		UI::Dockspace dockspace;

		MyEditor* editor;

		RendererAPI::FrameBuffer* fbo = nullptr;
		RendererAPI::Pipeline* pipeline = nullptr;
		RendererAPI::Camera* camera = nullptr;
		Mesh* mesh = nullptr;
	};
}

TerraForge3D::Application* CreateApplication()
{
	return new TerraForge3D::MainApplication();
}