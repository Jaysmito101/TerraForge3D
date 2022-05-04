#include "Utils/Utils.hpp"



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

		}
	}

}