#include "Base/Core/Core.hpp"

#include "vulkan/vulkan.h"

#ifdef TF3D_DEBUG
#define TF3D_VK_CALL(x) TF3D_ASSERT(x == VK_SUCCESS, "Vulkan call failed: " + std::string(__PRETTY_FUNCTION__) + ": " + std::to_string(x))
#else
#define TF3D_VK_CALL(x) x;
#endif