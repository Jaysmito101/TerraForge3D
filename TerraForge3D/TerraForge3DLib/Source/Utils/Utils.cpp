#include "Utils/Utils.hpp"

#include <chrono>
#include <format>
#include <iostream>


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
	}

}