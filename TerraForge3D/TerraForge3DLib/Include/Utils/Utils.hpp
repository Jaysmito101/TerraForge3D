#pragma once
#include "Base/Core/Core.hpp"

namespace TerraForge3D
{

	namespace Utils
	{

		std::string GetExecutablePath();

		std::string GetExecetableDirectory();

		std::string GetTimeStamp();

		std::string ReadTextFile(std::string filepath, bool* success = nullptr);

		bool WriteTextFile(std::string filepath, std::string contents, bool* success = nullptr);

	}

}
