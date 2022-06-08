#include "UI/Dockspace.hpp"
#include "TerraForge3D.hpp"

namespace TerraForge3D
{

	namespace UI
	{

		void Dockspace::Begin()
		{
			if (optFullScreen)
			{
				ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(viewport->Pos);
				ImGui::SetNextWindowSize(viewport->Size);
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.1f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
				windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
			}


			if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
			{
				windowFlags |= ImGuiWindowFlags_NoBackground;
			}

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("DockSpace", &isOpen, windowFlags);
			ImGui::PopStyleVar();

			if (optFullScreen)
			{
				ImGui::PopStyleVar(2);
			}

			ImGuiIO& io = ImGui::GetIO();
			ImGuiStyle& style = ImGui::GetStyle();
			float minWinSizeX = style.WindowMinSize.x;
			style.WindowMinSize.x = 370.f;

			if (io.ConfigFlags && ImGuiConfigFlags_DockingEnable)
			{
				ImGuiID dockSpaceID = ImGui::GetID(uid.ToString().c_str());
				ImGui::DockSpace(dockSpaceID, ImVec2(0.0f, 0.0f), dockspaceFlags);
			}

			style.WindowMinSize.x = minWinSizeX;
		}

		void Dockspace::End()
		{
			ImGui::End();
		}

	}

}