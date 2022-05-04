#include "UI/Style.hpp"
#include "Utils/Utils.hpp"

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
				auto& v = it.value();
				TF3D_ASSERT(v["Index"] > 0 && v["Index"] < ImGuiCol_COUNT, "Invalid ImGuiCol Index");
				style.Colors[v["Index"]] = ImVec4(v["R"], v["G"], v["B"], v["A"]);
			}

			// Floats
			style.Alpha = st["Alpha"];
			style.DisabledAlpha = st["DisabledAlpha"];
			style.WindowRounding = st["WindowRounding"];
			style.WindowBorderSize = st["WindowBorderSize"];
			style.ChildRounding = st["ChildRounding"];
			style.ChildBorderSize = st["ChildBorderSize"];
			style.PopupRounding = st["PopupRounding"];
			style.PopupBorderSize = st["PopupBorderSize"];
			style.FrameRounding = st["FrameRounding"];
			style.FrameBorderSize = st["FrameBorderSize"];
			style.IndentSpacing = st["IndentSpacing"];
			style.ColumnsMinSpacing = st["ColumnsMinSpacing"];
			style.ScrollbarSize = st["ScrollbarSize"];
			style.ScrollbarRounding = st["ScrollbarRounding"];
			style.GrabMinSize = st["GrabMinSize"];
			style.GrabRounding = st["GrabRounding"];
			style.LogSliderDeadzone = st["LogSliderDeadzone"];
			style.TabRounding = st["TabRounding"];
			style.TabBorderSize = st["TabBorderSize"];
			style.TabMinWidthForCloseButton = st["TabMinWidthForCloseButton"];
			style.MouseCursorScale = st["MouseCursorScale"];
			style.AntiAliasedLines = st["AntiAliasedLines"];
			style.AntiAliasedLinesUseTex = st["AntiAliasedLinesUseTex"];
			style.AntiAliasedFill = st["AntiAliasedFill"];
			style.CurveTessellationTol = st["CurveTessellationTol"];
			style.CircleTessellationMaxError = st["CircleTessellationMaxError"];

			// Directions (int)
			style.ColorButtonPosition = static_cast<ImGuiDir>(st["ColorButtonPosition"].get<int>());
			style.WindowMenuButtonPosition = static_cast<ImGuiDir>(st["WindowMenuButtonPosition"].get<int>());

			// ImVec2
			style.ButtonTextAlign = ImVec2(st["ButtonTextAlign"]["X"], st["ButtonTextAlign"]["Y"]);
			style.SelectableTextAlign = ImVec2(st["SelectableTextAlign"]["X"], st["SelectableTextAlign"]["Y"]);
			style.DisplayWindowPadding = ImVec2(st["DisplayWindowPadding"]["X"], st["DisplayWindowPadding"]["Y"]);
			style.DisplaySafeAreaPadding = ImVec2(st["DisplaySafeAreaPadding"]["X"], st["DisplaySafeAreaPadding"]["Y"]);
			style.ItemSpacing = ImVec2(st["ItemSpacing"]["X"], st["ItemSpacing"]["Y"]);
			style.ItemInnerSpacing = ImVec2(st["ItemInnerSpacing"]["X"], st["ItemInnerSpacing"]["Y"]);
			style.CellPadding = ImVec2(st["CellPadding"]["X"], st["CellPadding"]["Y"]);
			style.TouchExtraPadding = ImVec2(st["TouchExtraPadding"]["X"], st["TouchExtraPadding"]["Y"]);
			style.WindowPadding = ImVec2(st["WindowPadding"]["X"], st["WindowPadding"]["Y"]);
			style.WindowMinSize = ImVec2(st["WindowMinSize"]["X"], st["WindowMinSize"]["Y"]);
			style.WindowTitleAlign = ImVec2(st["WindowTitleAlign"]["X"], st["WindowTitleAlign"]["Y"]);
			style.FramePadding = ImVec2(st["FramePadding"]["X"], st["FramePadding"]["Y"]);

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