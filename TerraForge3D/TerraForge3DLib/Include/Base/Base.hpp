#pragma once

// Window Includes
#include "Base/Window/Window.hpp"

// The Core Includes
#include "Base/Core/Core.hpp"
#include "Base/Core/Application.hpp"

// Math Includes
#include "Base/Math/Math.hpp"

// Renderer
#include "Base/Renderer/Renderer.hpp"
#ifndef TF3D_EXCLUDE_RENDERER_API
#include "Base/Renderer/GPUTexture.hpp"
#include "Base/Renderer/Context.hpp"
#include "Base/Renderer/UIManager.hpp"
#endif

// Data Strutures
#ifndef TF3D_EXCLUDE_DATA_STRCUTRUES
#include "Base/DS/DS.hpp"
#endif

// ImGui
#include "imgui/imgui.h"