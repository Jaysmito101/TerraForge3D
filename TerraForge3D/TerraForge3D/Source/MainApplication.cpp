#include "TerraForge3D.hpp"
#include "EntryPoint.hpp"


// Temp
#include "Base/DS/DS.hpp"

namespace TerraForge3D
{
	class MainApplication : public Application
	{
	public:
		virtual void OnPreload()
		{
			appState = ApplicationState::Create();
			appState->core.app = this;
			appState->core.window = GetWindow();
			SetLogFilePath(appState->appResourcePaths.currentLogFilePath);
			SetApplicationName(appState->meta.name);
			SetWindowConfigPath(appState->appResourcePaths.windowConfigPath);
		}

		virtual void OnStart() override
		{
			TF3D_LOG("Started Application!");
			exitcb = GetInputEventManager()->RegisterCallback([&](InputEventParams* params) -> bool {
				TF3D_LOG_INFO("Shutting down application.");
				Close();
				return true;
				}, { InputEventType_WindowClose });
			tex = new Texture2D();
			tex->SetStorageMode(TextureStorageMode_GPUOnly);
			tex->LoadFromFile("sample.png");

		}

		virtual void OnUpdate() override
		{
			if (InputSystem::IsKeyPressedS(KeyCode_Escape))
			{
				Close();
			}
			//TF3D_LOG("Mouse Pos {0} , {1}", InputSystem::GetMouseDeltaS().first, InputSystem::GetMouseDeltaS().second)
		}

		virtual void OnImGuiRender() override
		{

			Renderer::Get()->uiManager->clearColor[0] = pos[0]/ 600;
			
			static bool isTestRunning = false;
			static int testWindowCount = 10;

			if (isTestRunning)
			{
				for (int i = 0; i < testWindowCount;i++)
				{
					ImGui::Begin((std::string("Window : ")+std::to_string(i)).c_str());
					ImGui::Text("Test Window");
					ImGui::Button("Button");
					ImGui::Image(tex->GetImGuiID(), ImVec2(tex->GetWidth(), tex->GetHeight()));
					ImGui::ArrowButton("Arrow Button", ImGuiDir_Left);
					ImGui::Image(tex->GetImGuiID(), ImVec2(200, 200));
					ImGui::End();
				}
			}


			ImGui::Begin("Hello World!");

			ImGui::Text("Welcome to TerraForge3D!");
			ImGui::Text("Window Position: %f, %f", pos[0], pos[1]);
			ImGui::Text("Draw Window with middle mouse button to move it!");
			
			ImGui::NewLine();
			
			ImGui::Text("Delta Time: %f", deltaTime);
			ImGui::Text("Frame Rate: %d", static_cast<uint32_t>(1000.0 / deltaTime));
			
			ImGui::NewLine();

			if (ImGui::Button("Save Style"))
			{
				style.LoadCurrent();
				style.SaveToFile(appState->appResourcePaths.stylesDir + PATH_SEPERATOR "style.json");
			}

			ImGui::NewLine();
			
			ImGui::Checkbox("Stress Test", &isTestRunning);
			ImGui::InputInt("Stress Test Window Count", &testWindowCount, 5);
			
			ImGui::DragFloat2("UV", coord, 0.01, 0.0, 1.0);
			ImGui::ColorEdit4("Color", col);
			if (ImGui::Button("SetPixel"))
				tex->SetPixel(coord[0], coord[1], col[0], col[1], col[2], col[3]);
			if (ImGui::Button("GetPixel"))
				tex->GetPixel(coord[0], coord[1], &col[0], &col[1], &col[2], &col[3]);

			static char dataT[4096] = {0};
			ImGui::InputTextWithHint("Path", "Enter Path of Image to Load", dataT, 4096);
			if (ImGui::Button("Load"))
			{

				tex->Destroy();
				tex->LoadFromFile(std::string(dataT));

			}

			ImGui::Image(tex->GetImGuiID(), ImVec2(200, 200));
			if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right))
			{
				auto [xm, ym] = ImGui::GetMousePos();
				auto [xi, yi] = ImGui::GetItemRectMin();
				float x = std::clamp(xm - xi, 0.0f, 200.0f) / 200.0f +  ((float)rand() / RAND_MAX) * 0.05;
				float y = std::clamp(ym - yi, 0.0f, 200.0f) / 200.0f + ((float)rand() / RAND_MAX) * 0.05;
				for (float i = std::clamp(x - 0.1f, 0.0f, 1.0f); i < std::clamp(x + 0.1f, 0.0f, 1.0f); i += 0.03f)
				{
					for (float j = std::clamp(y - 0.1f, 0.0f, 1.0f); j < std::clamp(y + 0.1f, 0.0f, 1.0f); j += 0.03f)
					{
						if(std::abs((x-i)*(x-i) + (y-j)*(y-j)) < 1)
							tex->SetPixel(i, j, col[0], col[1], col[2], col[3]);
					}
				}
			}

			ImGui::Text("Data Type: %s\nChannels: %s\nWidth: %d\nHeight: %d", ToString(tex->GetDataType()).c_str(), ToString(tex->GetChannels()).c_str(), tex->GetWidth(), tex->GetHeight());

			if (ImGui::Button("Exit"))
				Close();

			ImGui::End();

			ImGui::ShowStyleEditor();
			ImGui::ShowDemoWindow();


		}

		virtual void OnEnd() override
		{
			TF3D_LOG("Started Shutdown!");
			GetInputEventManager()->DeregisterCallback(exitcb);
			ApplicationState::Destory();
			
			TF3D_SAFE_DELETE(tex);
		}

	private:
		uint32_t exitcb;
		float pos[2] = {0.0f};
		float col[4] = {1.0f, 1.0, 1.0, 1.0};
		float coord[2] = {0.0f};

		Texture2D* tex = nullptr;
		ApplicationState* appState = nullptr;
		UI::Style style;

	};
}

TerraForge3D::Application* CreateApplication()
{
	return new TerraForge3D::MainApplication();
}