#pragma once

#include "Base/Core/Assert.hpp"
#include "Base/Core/Logger.hpp"
#include "Base/Core/UUID.hpp"

// STL Includes
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <iostream>
#include <sstream>
#include <functional>
#include <cstdint>


// Macros

#define TF3D_GENERATION 3
#define TF3D_VERSION_MIN 0
#define TF3D_VERSION_MAX 0

#define TF3D_VERSION_STRING ( std::to_string(TF3D_GENERATION) + "." + std::to_string(TF3D_VERSION_MIN) + "." + std::to_string(TF3D_VERSION_MAX) )


#define TF3D_SAFE_DELETE(x) delete x; x = nullptr;

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif