#include "Base/Core/Assert.hpp"

#ifdef TF3D_DEBUG

#include "Base/Core/Logger.hpp"

namespace TerraForge3D
{
	void Assert(bool condition, std::string message)
	{
		if(!condition)
		{
			TF3D_LOG_ERROR("ASSERTION FAILED : {0}", message);
#ifdef TF3D_WINDOWS
			__debugbreak();
#else
			throw std::exception("ASSEERTION FALIURE");
#endif
		}
	}
}

#endif