#include "UI/Style.hpp"
#include "TerraForge3D.hpp"

#include "imgui/imgui.h"
#include "json/json.hpp"

namespace TerraForge3D
{

	namespace UI
	{



		Style::Style()
		{
		}

		Style::~Style()
		{
		}

		void Style::LoadFromFile(std::string filepath)
		{
			bool loaded = false;
			std::string contents = Utils::ReadTextFile(filepath, &loaded);
			if (loaded)
				LoadFormString(contents);
			else
			{
				TF3D_LOG_ERROR("Failed to load style file from {0}", filepath);
			}
		}

		void Style::LoadFormString(std::string contents)
		{
			nlohmann::json st;
			try
			{
				st = nlohmann::json::parse(contents);
			}
			catch (std::exception& e)
			{
				TF3D_LOG_ERROR("Failed to parse style JSON ({0})", e.what());
			}

			// Load Colors
			nlohmann::json& colors = st["Colors"];
			for (auto it = colors.begin(); it != colors.end(); it++)
			{
				auto& value = it.value()["Value"];
				int index = it.value()["Index"].get<int>();
				TF3D_ASSERT(index >= 0 && index < ImGuiCol_COUNT, std::string("Invalid ImGuiCol Index ") + std::to_string(index));
				TF3D_HANDLE_EXCEPTION_MSG(style.Colors[index] = ImVec4(value["R"], value["G"], value["B"], value["A"]);, std::string("Error in loading color forom : ") + value.dump(4));
			}

			TF3D_HANDLE_EXCEPTION_MSG(name = st["Name"];, "Error in loading name from style");

			// Floats
			TF3D_HANDLE_EXCEPTION_MSG(style.Alpha = st["Alpha"];, "Error in loading Alpha from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.DisabledAlpha = st["DisabledAlpha"]; , "Error in loading DisabledAlpha from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.WindowRounding = st["WindowRounding"]; , "Error in loading WindowRounding from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.WindowBorderSize = st["WindowBorderSize"]; , "Error in loading WindowBorderSize from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.ChildRounding = st["ChildRounding"]; , "Error in loading ChildRounding from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.ChildBorderSize = st["ChildBorderSize"]; , "Error in loading ChildBorderSize from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.PopupRounding = st["PopupRounding"]; , "Error in loading PopupRounding from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.PopupBorderSize = st["PopupBorderSize"]; , "Error in loading PopupBorderSize from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.FrameRounding = st["FrameRounding"]; , "Error in loading FrameRounding from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.FrameBorderSize = st["FrameBorderSize"]; , "Error in loading FrameBorderSize from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.IndentSpacing = st["IndentSpacing"]; , "Error in loading IndentSpacing from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.ColumnsMinSpacing = st["ColumnsMinSpacing"]; , "Error in loading ColumnsMinSpacing from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.ScrollbarSize = st["ScrollbarSize"]; , "Error in loading ScrollbarSize from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.ScrollbarRounding = st["ScrollbarRounding"]; , "Error in loading ScrollbarRounding from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.GrabMinSize = st["GrabMinSize"]; , "Error in loading GrabMinSize from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.GrabRounding = st["GrabRounding"]; , "Error in loading GrabRounding from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.LogSliderDeadzone = st["LogSliderDeadzone"]; , "Error in loading LogSliderDeadzone from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.TabRounding = st["TabRounding"]; , "Error in loading TabRounding from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.TabBorderSize = st["TabBorderSize"]; , "Error in loading TabBorderSize from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.TabMinWidthForCloseButton = st["TabMinWidthForCloseButton"]; , "Error in loading TabMinWidthForCloseButton from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.MouseCursorScale = st["MouseCursorScale"]; , "Error in loading MouseCursorScale from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.AntiAliasedLines = st["AntiAliasedLines"]; , "Error in loading AntiAliasedLines from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.AntiAliasedLinesUseTex = st["AntiAliasedLinesUseTex"]; , "Error in loading AntiAliasedLinesUseTex from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.AntiAliasedFill = st["AntiAliasedFill"]; , "Error in loading AntiAliasedFill from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.CurveTessellationTol = st["CurveTessellationTol"]; , "Error in loading CurveTessellationTol from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.CircleTessellationMaxError = st["CircleTessellationMaxError"]; , "Error in loading CircleTessellationMaxError from style");

			// Directions (int)
			TF3D_HANDLE_EXCEPTION_MSG(style.ColorButtonPosition = static_cast<ImGuiDir>(st["ColorButtonPosition"].get<int>());, "Error in loading ColorButtonPosition from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.WindowMenuButtonPosition = static_cast<ImGuiDir>(st["WindowMenuButtonPosition"].get<int>()), "Error in loading WindowMenuButtonPosition from style");

			// ImVec2
			TF3D_HANDLE_EXCEPTION_MSG(style.ButtonTextAlign = ImVec2(st["ButtonTextAlign"]["X"], st["ButtonTextAlign"]["Y"]); , "Error in loading ButtonTextAlign  from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.SelectableTextAlign = ImVec2(st["SelectableTextAlign"]["X"], st["SelectableTextAlign"]["Y"]);, "Error in loading SelectableTextAlign  from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.DisplayWindowPadding = ImVec2(st["DisplayWindowPadding"]["X"], st["DisplayWindowPadding"]["Y"]);, "Error in loading DisplayWindowPadding  from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.DisplaySafeAreaPadding = ImVec2(st["DisplaySafeAreaPadding"]["X"], st["DisplaySafeAreaPadding"]["Y"]);, "Error in loading DisplaySafeAreaPadding  from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.ItemSpacing = ImVec2(st["ItemSpacing"]["X"], st["ItemSpacing"]["Y"]);, "Error in loading ItemSpacing  from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.ItemInnerSpacing = ImVec2(st["ItemInnerSpacing"]["X"], st["ItemInnerSpacing"]["Y"]);, "Error in loading ItemInnerSpacing  from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.CellPadding = ImVec2(st["CellPadding"]["X"], st["CellPadding"]["Y"]);, "Error in loading CellPadding  from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.TouchExtraPadding = ImVec2(st["TouchExtraPadding"]["X"], st["TouchExtraPadding"]["Y"]);, "Error in loading TouchExtraPadding  from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.WindowPadding = ImVec2(st["WindowPadding"]["X"], st["WindowPadding"]["Y"]);, "Error in loading WindowPadding  from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.WindowMinSize = ImVec2(st["WindowMinSize"]["X"], st["WindowMinSize"]["Y"]);, "Error in loading WindowMinSize  from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.WindowTitleAlign = ImVec2(st["WindowTitleAlign"]["X"], st["WindowTitleAlign"]["Y"]);, "Error in loading WindowTitleAlign  from style");
			TF3D_HANDLE_EXCEPTION_MSG(style.FramePadding = ImVec2(st["FramePadding"]["X"], st["FramePadding"]["Y"]); , "Error in loading FramePadding  from style");

		}

		void Style::LoadCurrent()
		{
			style = ImGui::GetStyle();
		}

		void Style::SaveToFile(std::string filepath)
		{
			std::string contents = SaveToString();
			if (!Utils::WriteTextFile(filepath, contents))
			{
				TF3D_LOG_ERROR("Failed to save style to {0}", filepath);
			}
		}

		std::string Style::SaveToString()
		{
			nlohmann::json st;
			nlohmann::json colors;
			nlohmann::json color;
			nlohmann::json colorValue;
			nlohmann::json tmp;
			for (int i = 0; i < ImGuiCol_COUNT; i++)
			{
				color["Name"] = ImGui::GetStyleColorName(i);
				color["Index"] = i;
				colorValue["R"] = style.Colors[i].x;
				colorValue["G"] = style.Colors[i].y;
				colorValue["B"] = style.Colors[i].z;
				colorValue["A"] = style.Colors[i].w;
				color["Value"] = colorValue;
				colors[color["Name"]] = color;
			}

			st["Name"] = name;

			st["Colors"] = colors;

			// Floats
			st["Alpha"] = style.Alpha;
			st["DisabledAlpha"] = style.DisabledAlpha;
			st["WindowRounding"] = style.WindowRounding;
			st["WindowBorderSize"] = style.WindowBorderSize;
			st["ChildRounding"] = style.ChildRounding;
			st["ChildBorderSize"] = style.ChildBorderSize;
			st["PopupRounding"] = style.PopupRounding;
			st["PopupBorderSize"] = style.PopupBorderSize;
			st["FrameRounding"] = style.FrameRounding;
			st["FrameBorderSize"] = style.FrameBorderSize;
			st["IndentSpacing"] = style.IndentSpacing;
			st["ColumnsMinSpacing"] = style.ColumnsMinSpacing;
			st["ScrollbarSize"] = style.ScrollbarSize;
			st["ScrollbarRounding"] = style.ScrollbarRounding;
			st["GrabMinSize"] = style.GrabMinSize;
			st["GrabRounding"] = style.GrabRounding;
			st["LogSliderDeadzone"] = style.LogSliderDeadzone;
			st["TabRounding"] = style.TabRounding;
			st["TabBorderSize"] = style.TabBorderSize;
			st["TabMinWidthForCloseButton"] = style.TabMinWidthForCloseButton;
			st["MouseCursorScale"] = style.MouseCursorScale;
			st["AntiAliasedLines"] = style.AntiAliasedLines;
			st["AntiAliasedLinesUseTex"] = style.AntiAliasedLinesUseTex;
			st["AntiAliasedFill"] = style.AntiAliasedFill;
			st["CurveTessellationTol"] = style.CurveTessellationTol;
			st["CircleTessellationMaxError"] = style.CircleTessellationMaxError;

			// Directions (int)
			st["ColorButtonPosition"] = static_cast<int>(style.ColorButtonPosition);
			st["WindowMenuButtonPosition"] = static_cast<int>(style.WindowMenuButtonPosition);

			// ImVec2
			tmp["X"] = style.ButtonTextAlign.x;
			tmp["Y"] = style.ButtonTextAlign.y;
			st["ButtonTextAlign"] = tmp;
			tmp["X"] = style.SelectableTextAlign.x;
			tmp["Y"] = style.SelectableTextAlign.y;
			st["SelectableTextAlign"] = tmp;
			tmp["X"] = style.DisplayWindowPadding.x;
			tmp["Y"] = style.DisplayWindowPadding.y;
			st["DisplayWindowPadding"] = tmp;
			tmp["X"] = style.DisplaySafeAreaPadding.x;
			tmp["Y"] = style.DisplaySafeAreaPadding.y;
			st["DisplaySafeAreaPadding"] = tmp;
			tmp["X"] = style.ItemSpacing.x;
			tmp["Y"] = style.ItemSpacing.y;
			st["ItemSpacing"] = tmp;
			tmp["X"] = style.ItemInnerSpacing.x;
			tmp["Y"] = style.ItemInnerSpacing.y;
			st["ItemInnerSpacing"] = tmp;
			tmp["X"] = style.CellPadding.x;
			tmp["Y"] = style.CellPadding.y;
			st["CellPadding"] = tmp;
			tmp["X"] = style.TouchExtraPadding.x;
			tmp["Y"] = style.TouchExtraPadding.y;
			st["TouchExtraPadding"] = tmp;
			tmp["X"] = style.WindowPadding.x;
			tmp["Y"] = style.WindowPadding.y;
			st["WindowPadding"] = tmp;
			tmp["X"] = style.WindowMinSize.x;
			tmp["Y"] = style.WindowMinSize.y;
			st["WindowMinSize"] = tmp;
			tmp["X"] = style.WindowTitleAlign.x;
			tmp["Y"] = style.WindowTitleAlign.y;
			st["WindowTitleAlign"] = tmp;
			tmp["X"] = style.FramePadding.x;
			tmp["Y"] = style.FramePadding.y;
			st["FramePadding"] = tmp;

			return st.dump(1, '\t');
		}

		void Style::Apply()
		{
			ImGuiStyle& st = ImGui::GetStyle();
			st = style;
		}

	}

}