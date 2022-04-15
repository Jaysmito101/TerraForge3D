#include "Base/Core/Assert.hpp"

#ifdef TF3D_DEBUG

#include "Base/Core/Logger.hpp"

namespace TerraForge3D
{
	void Assert(bool condition, std::string message, std::string file, std::string line, std::string function)
	{
		if(!condition)
		{
			TF3D_LOG_ERROR("ASSERTION FAILED : {0}\nFile: {1}\nLine: {2}\nfunction: {3}", message, file, line, function);
#ifdef TF3D_WINDOWS
			__debugbreak();
#else
			throw std::exception("ASSEERTION FALIURE");
#endif
		}
	}
}

#endif