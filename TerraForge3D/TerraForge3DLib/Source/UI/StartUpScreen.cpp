#include "UI/StartUpScreen.hpp"
#include "UI/Modals.hpp"
#include "Data/ApplicationState.hpp"
#include "Data/ProjectManager.hpp"
#include "Utils/Utils.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"

namespace TerraForge3D
{
	StartUpScreen::StartUpScreen(ApplicationState* appState)
	{
		this->appState = appState;
	}

	StartUpScreen::~StartUpScreen()
	{

	}

	void StartUpScreen::OnUpdate()
	{

	}

	void StartUpScreen::OnShow()
	{
		ImGui::SetWindowSize(ImGui::GetWindowViewport()->Size);
		ImGui::SetWindowFontScale(1.3f);

		ImGui::SetCursorPos(ImGui::GetWindowSize() * 0.5 - ImVec2(200.0f, 200.0f));

		ImGui::BeginChild("##StartUpScreen", ImVec2(400.0f, 400.0f));
		
		ImGui::Text("Welcome to TerraForge3D!");
		ImGui::NewLine();
		Utils::ImGuiC::TextIcon(ICON_MD_FOLDER_OPEN);
		if (ImGui::Button("Open Project"))
		{
			appState->modals.manager->FileDialog("Open Project", openFDI);
		}
		ImGui::NewLine();
		Utils::ImGuiC::TextIcon(ICON_MD_CREATE_NEW_FOLDER);
		if (ImGui::Button("Create Project"))
		{
			appState->modals.manager->FileDialog("Create Project", createFDI);
		}
		ImGui::NewLine();
		ImGui::Text("Tip of the day: {%s}", "Yet to be added.");

		ImGui::EndChild();

		ImGui::SetWindowPos(ImVec2(0,0));
	}

	void StartUpScreen::OnStart()
	{
		windowFlags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

		openFDI.mode = UI::FileDialogMode_Open;
		openFDI.selection = UI::FileDialogSelection_DirectoriesOnly;
		openFDI.onSelect = [this](UI::FileDialogInfo* info) -> void
		{
			if (appState->project.manager->Load(info->selectedFilePath))
			{
				isVisible = false;
			}
		};

		createFDI.mode = UI::FileDialogMode_Save;
		createFDI.selection = UI::FileDialogSelection_DirectoriesOnly;
		createFDI.onSelect = [this](UI::FileDialogInfo* info) -> void
		{
			if (appState->project.manager->Create(info->selectedFilePath))
			{
				isVisible = false;
			}
		};
	}

	void StartUpScreen::OnEnd()
	{

	}
}