#include "TerraForge3D.hpp"
#include "EntryPoint.hpp"

#include "IconsMaterialDesign.h"

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
	}

	void OnUIRender() override
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

		if (ImGui::Button("Exit"))
			appState->core.app->Close();

	}

	void OnStart() override
	{
		TF3D_LOG("On Start");
		appState->menus.mainMenu->GetManagerPTR()->Register("Windows/Style Opener", [this](TerraForge3D::UI::MenuItem* context) {
			isVisible = (context->GetToggleState());
			context->RegisterTogglePTR(&isVisible);
			}, TerraForge3D::UI::MenuItemUse_Toggle);
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
			appState->editors.manager = new UI::EditorManager("Primary Editor Manager");


			editor = new MyEditor("Style Opener", appState);
			appState->editors.manager->AddEditor(editor);
		}

		virtual void OnUpdate() override
		{
			appState->editors.manager->Update();
		}

		virtual void OnImGuiRender() override
		{
			dockspace.Begin();
 
			appState->menus.mainMenu->Show();
			appState->editors.manager->RenderUI();

			dockspace.End();
		}

		virtual void OnEnd() override
		{
			TF3D_LOG("Started Shutdown!");
			GetInputEventManager()->DeregisterCallback(exitcb);
			
			// Object Destructions
			TF3D_SAFE_DELETE(appState->menus.mainMenu);
			TF3D_SAFE_DELETE(appState->editors.manager);

			ApplicationState::Destory();
		}

	private:
		uint32_t exitcb;
		float pos[2] = {0.0f};

		ApplicationState* appState = nullptr;
		UI::Dockspace dockspace;

		MyEditor* editor;
	};
}

TerraForge3D::Application* CreateApplication()
{
	return new TerraForge3D::MainApplication();
}