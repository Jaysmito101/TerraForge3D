#pragma once
#include "Base/Core/Core.hpp"
#include "IconsMaterialDesign.h"
#include "imgui/imgui.h"

struct ImFont;
struct ImVec4;

namespace TerraForge3D
{

	namespace Utils
	{

		std::string GetExecutablePath();

		std::string GetExecetableDirectory();

		std::string GetTimeStamp();

		std::string ReadTextFile(std::string filepath, bool* success = nullptr);

		bool WriteTextFile(std::string filepath, std::string contents, bool* success = nullptr);

		bool PathExists(std::string path, bool requireWrite = true);

		bool MkDir(std::string path);

		bool RemoveFile(std::string path);

		bool RmDir(std::string path);

		void SleepFor(uint64_t duration);

		enum FileDialogSelection
		{
			FileDialogSelection_File,
			FileDialogSelection_Directory,
			FileDialogSelection_FileAndDirectory
		};

		namespace ImGuiC
		{
			void SetIconFont(void* font);

			bool ButtonIcon(const char* icon, const char* text = nullptr, bool newline = true);

			void TextIcon(const char* icon, bool newline = false);

			void ColoredTextIcon(const char* icon, ImVec4 color, bool newline = false);

			void PushSubFont(ImFont* font);

			void PopSubFont();

			template <typename Type>
			bool ComboBox(const char* id, Type* current, const char** list, uint32_t listCount)
			{
				Type oldValue = *current;
				if (ImGui::BeginCombo(id, list[static_cast<uint32_t>(*current)]))
				{
					for (uint32_t n = 0; n < listCount; n++)
					{
						bool isSelected = (list[static_cast<uint32_t>(*current)] == list[n]);
						if (ImGui::Selectable(list[n], isSelected))
							*current = static_cast<Type>(n);
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				return oldValue != *current;
			}
		}

		namespace String
		{
			std::vector<std::string> Split(std::string str, std::vector<std::string> delimeters = { " ", "\t", "\n" }, bool includeDelimeteres = false);
			
			template<typename T>
			std::string ToString(const std::vector<T>& v)
			{
				std::stringstream os;
				os << "[";
				for (int i = 0; i < v.size(); i++)
				{
					os << v[i] << (i != v.size() - 1 ? ", " : "");
				}
				os << "]";
				return os.str();
			}
		}


	}

}
