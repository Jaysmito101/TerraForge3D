#include "Utils/Utils.hpp"
#include "TerraForge3D.hpp"

#include "imgui/imgui.h"

#include <filesystem>
#include <stack>

#ifdef TF3D_WINDOWS

#include <Windows.h>

#elif defined(TF3D_LINUX)

#elif defined(TF3D_MACOSX)

#endif

namespace TerraForge3D
{

	namespace Utils
	{
		std::string GetExecutablePath()
		{
			char rawPathName[MAX_PATH];
#ifdef TF3D_WINDOWS
			GetModuleFileNameA(NULL, rawPathName, MAX_PATH);
#else
			readlink("/proc/self/exe", rawPathName, PATH_MAX);
#endif
			return std::string(rawPathName);
		}

		std::string GetExecetableDirectory()
		{
			std::string executablePath = GetExecutablePath();
			std::string directory = executablePath.substr(0, executablePath.find_last_of("\\/"));
			return directory;
		}

		// From https://stackoverflow.com/a/70332391/14911094
		std::string GetTimeStamp()
		{
			const auto now = std::chrono::system_clock::now();
			return std::format("{:%d-%m-%Y_%H-%M-%OS}", now);
		}

		std::string ReadTextFile(std::string filepath, bool* success)
		{
			try {
				std::ifstream f;
				f.open(filepath);

				if (f.is_open() && f.good())
				{
					std::stringstream s;
					s << f.rdbuf();
					f.close();
					if (success)
						*success = true;
					return s.str();
				}
				else
				{
					if (success)
						*success = false;
					return "";
				}
			}
			catch (std::exception& e)
			{
				TF3D_LOG_ERROR("Error while reading text file {0}, ERROR: {1}", filepath, e.what());
				if (success)
					*success = false;
				return "";
			}
		}

		bool WriteTextFile(std::string filepath, std::string contents, bool* success)
		{
			try {
				std::ofstream f;
				f.open(filepath);

				if (f.is_open() && f.good())
				{
					f << contents;
					f.close();
					if (success)
						*success = true;
					return true;
				}
				else
				{
					if (success)
						*success = false;
					return false;
				}
			}
			catch (std::exception& e)
			{
				TF3D_LOG_ERROR("Error while writing text file {0}, ERROR: {1}", filepath, e.what());
				if (success)
					*success = false;
				return false;
			}
			return true;
		}

		bool PathExists(std::string path, bool requireWrite)
		{
			if (std::filesystem::exists(path))
			{
				// TODO: Check for write permissions
				return true;
			}
			return false;
		}

		bool MkDir(std::string path)
		{
			if (!PathExists(path))
			{
				system((std::string("mkdir -p \"") + path + "\"").c_str());
			}
			return true;
		}

		bool RemoveFile(std::string path)
		{
			if (!PathExists(path))
				return false;
			try {
				if (std::filesystem::remove(path))
					return true;
				else
					return false;
			}
			catch (const std::filesystem::filesystem_error& err) {
				TF3D_LOG_ERROR("Filesystem Error : ", err.what());
				return false;
			}
			return true;
		}

		bool RmDir(std::string path)
		{
			if (!PathExists(path))
				return false;
			try {

				if (std::filesystem::remove_all(path))
					return true;
				else
					return false;
			}
			catch (const std::filesystem::filesystem_error& err) {
				TF3D_LOG_ERROR("Filesystem Error : ", err.what());
				return false;
			}
			return true;
		}

		void SleepFor(uint64_t duration)
		{
#ifdef TF3D_WINDOWS
			Sleep((DWORD)duration);
#elif defined(TF3D_LINUX)
			usleep(duration);
#elif defined(TF3D_MACOSX)
			TF3D_ASSERT(false, "Not yet implemented");
#endif
		}

		namespace ImGuiC
		{
			static ImFont* font = nullptr;
			static std::stack<ImFont*> subFonts;
			static char buffer[4096] = {0};

			void SetIconFont(void* f)
			{
				font = reinterpret_cast<ImFont*>(f);
			}

			bool ButtonIcon(const char* icon, const char* text, bool newline)
			{
				bool iconLoaded = font->IsLoaded();
				if(iconLoaded)
					ImGui::PushFont(font);
				bool pressed = ImGui::Button(icon);
				if (iconLoaded)
					ImGui::PopFont();
				if (text)
				{
					if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
					{
						ImGui::SetTooltip(text);
					}
				}
				if (!newline)
					ImGui::SameLine();
				return pressed;
			}

			void TextIcon(const char* icon, bool newline)
			{		
				bool iconLoaded = font->IsLoaded();
				if (iconLoaded)
					ImGui::PushFont(font);
				ImGui::Text(icon);
				if (!newline)
					ImGui::SameLine();
				if (iconLoaded)
					ImGui::PopFont();
			}

			void ColoredTextIcon(const char* icon, ImVec4 color, bool newline)
			{
				bool iconLoaded = font->IsLoaded();
				if (iconLoaded)
					ImGui::PushFont(font);
				ImGui::TextColored(color, icon);
				if (!newline)
					ImGui::SameLine();
				if (iconLoaded)
					ImGui::PopFont();
			}

			void PushSubFont(ImFont* font)
			{
				subFonts.push(ImGui::GetFont());
				ImGui::PopFont();
				ImGui::PushFont(font);
			}

			void PopSubFont()
			{
				TF3D_ASSERT(subFonts.size() != 0, "Utils::ImGuiC::PopSubFont should only be called after Utils::ImGuiC::PushSubFont");
				ImGui::PopFont();
				ImGui::PushFont(subFonts.top());
				subFonts.pop();
			}
		}

		namespace String
		{

			std::vector<std::string> Split(std::string str, std::vector<std::string> delimeters, bool includeDelimeteres)
			{
				std::vector<std::string> result;
				std::string intermediate = "";
				for (size_t i = 0; i < str.size(); i++)
				{
					for (int j = 0; j < delimeters.size(); j++)
					{
						if (str.substr(i, delimeters[j].size()) == delimeters[j])
						{
							if (intermediate.size() > 0)
								result.push_back(intermediate);
							if (includeDelimeteres)
								result.push_back(delimeters[j]);
							intermediate = "";
							i += delimeters[j].size();
							break;
						}
					}
					intermediate += str[i];
				}
				if (intermediate.size() > 0)
					result.push_back(intermediate);
				return result;
			}


		}

	}

}

