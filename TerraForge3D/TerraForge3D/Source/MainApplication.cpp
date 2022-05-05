#include "TerraForge3D.hpp"
#include "EntryPoint.hpp"


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

			mainMenu.Register("File/Open", [](UI::MenuItem*) {exit(-1); })->SetShortcut("ABC")->SetEnabled(false);
			mainMenu.Register("File/Close", [](UI::MenuItem*) {exit(-1); });
			mainMenu.Register("File/New", [](UI::MenuItem*) {exit(-1); });
			mainMenu.Register("Edit/Preferences", [](UI::MenuItem*) {exit(-1); });
			mainMenu.Register("File/Recent/Abc.txt", [](UI::MenuItem*) {exit(-1); })->SetTooltip("HEllo")->SetEnabled(false);
			mainMenu.Register("File/Recent/BCt.txt", [](UI::MenuItem*) {exit(-1); });
			mainMenu.Register("Edit/Cut", [](UI::MenuItem*) {exit(-1); });
			mainMenu.Register("Edit/Replace", [](UI::MenuItem*) {exit(-1); });
			mainMenu.Register("Edit/Addons/Install", [](UI::MenuItem*) {exit(-1); });
			mainMenu.Register("Edit/Addons/Uninstall", [](UI::MenuItem*) {exit(-1); });
			mainMenu.Register("About/Website", [](UI::MenuItem*) {exit(-1); });
			mainMenu.Register("About/Youtube/Tutorials", [](UI::MenuItem*) {exit(-1); });
			mainMenu.Register("About/Youtube/Showcase", [](UI::MenuItem*) {exit(-1); });
			mainMenu.Register("About/Feedback", [](UI::MenuItem*) {exit(-1); });
			mainMenu.Register("Options/Select GPU", [](UI::MenuItem*) {exit(-1); });

			style.LoadFromFile(appState->appResourcePaths.stylesDir + PATH_SEPERATOR "Maya.json");
			style.Apply();
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

			mainMenu.Show();

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
			
			if (ImGui::Button("Exit"))
				Close();

			ImGui::End();

			ImGui::Begin("saddsadsa");
			ImGui::Text("sadsd");
			ImGui::End();

			dockspace.End();
		}

		virtual void OnEnd() override
		{
			TF3D_LOG("Started Shutdown!");
			GetInputEventManager()->DeregisterCallback(exitcb);
			ApplicationState::Destory();
		}

	private:
		uint32_t exitcb;
		float pos[2] = {0.0f};

		ApplicationState* appState = nullptr;
		UI::Style style;
		UI::Dockspace dockspace;		
		UI::Menu mainMenu;
	};
}

TerraForge3D::Application* CreateApplication()
{
	return new TerraForge3D::MainApplication();
}