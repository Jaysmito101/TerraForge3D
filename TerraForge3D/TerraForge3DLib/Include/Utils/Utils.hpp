#pragma once
#include "Base/Core/Core.hpp"
#include "IconsMaterialDesign.h"

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

		namespace ImGuiC
		{
			void SetIconFont(void* font);

			void TextIcon(const char* icon, bool newline = false);

			void ColoredTextIcon(const char* icon, ImVec4 color, bool newline = false);

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
