#pragma once

#include "Base/Core/Assert.hpp"
#include "Base/Core/Logger.hpp"
#include "Base/Core/UUID.hpp"

// STL Includes
#include <string>

// Macros

#define TF3D_GENERATION 3
#define TF3D_VERSION_MIN 0
#define TF3D_VERSION_MAX 0

#define TF3D_VERSION_STRING ( std::to_string(TF3D_GENERATION) + "." + std::to_string(TF3D_VERSION_MIN) + "." + std::to_string(TF3D_VERSION_MAX) )


#define TF3D_SAFE_DELETE(x) delete x; x = nullptr;