#pragma once

#include <string>
#include <stdexcept>

#include "Base/Core/Logger.hpp"

#ifdef TF3D_WINDOWS
#define TF3D_DEBUG_BREAK __debugbreak();
#else
#define TF3D_DEBUG_BREAK throw std::runtime_error("DEBUG BREAK");
#endif

// Disable assertions for Rlease Builds
#ifdef TF3D_DEBUG
#define TF3D_ASSERT(x, message) \
		if (!(x)) \
		{ \
			TF3D_LOG_ERROR("ASSERTION FAILED : {0}\n\tFile: {1}\n\tLine: {2}\n\tFunction: {3}", message, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
			TF3D_DEBUG_BREAK; \
		}
#else
#define TF3D_ASSERT(x, message)
#endif