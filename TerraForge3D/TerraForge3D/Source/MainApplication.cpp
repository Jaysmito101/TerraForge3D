#include "TerraForge3D.hpp"
#include "EntryPoint.hpp"

#include "IconsMaterialDesign.h"


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

			// Object Creations
			appState->menus.mainMenu = new MainMenu(appState);
		}

		virtual void OnUpdate() override
		{
			if (InputSystem::IsKeyPressedS(KeyCode_Escape))
			{
				Close();
			}
		}

		virtual void OnImGuiRender() override
		{
			dockspace.Begin();

			appState->menus.mainMenu->Show();

			Renderer::Get()->uiManager->clearColor[0] = pos[0]/ 600;
			ImGui::Begin("Hello World!");			
			ImGui::NewLine();			
			ImGui::Text("Delta Time: %f", deltaTime);
			ImGui::Text("Frame Rate: %d", static_cast<uint32_t>(1000.0 / deltaTime));			
			ImGui::NewLine();
			static char styleName[4096] = {};		
			ImGui::InputTextWithHint("Style Name", "Enter Style Name", styleName, 4096);
			if (ImGui::Button("Load Style"))
			{
				style.LoadFromFile(appState->appResourcePaths.stylesDir + PATH_SEPERATOR + std::string(styleName) + ".json");
				style.Apply();
			}
			ImGui::NewLine();
			ImGui::Text(ICON_MD_SAVE " Save");
			ImGui::Text(ICON_MD_FILE_COPY " File");
			ImGui::Text(ICON_MD_ANIMATION " Animation");
			

			if (ImGui::Button("Exit"))
				Close();

			ImGui::End();

			ImGui::Begin("saddsadsa");
			ImGui::Text("sadsd");
			ImGui::End();

			ImGui::ShowDemoWindow();

			dockspace.End();
		}

		virtual void OnEnd() override
		{
			TF3D_LOG("Started Shutdown!");
			GetInputEventManager()->DeregisterCallback(exitcb);
			
			// Object Destructions
			TF3D_SAFE_DELETE(appState->menus.mainMenu);


			ApplicationState::Destory();
		}

	private:
		uint32_t exitcb;
		float pos[2] = {0.0f};

		ApplicationState* appState = nullptr;
		UI::Style style;
		UI::Dockspace dockspace;
	};
}

TerraForge3D::Application* CreateApplication()
{
	return new TerraForge3D::MainApplication();
}