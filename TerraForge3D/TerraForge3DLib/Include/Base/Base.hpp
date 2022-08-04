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
#include "Base/Renderer/Framebuffer.hpp"
#include "Base/Renderer/NativeRenderer.hpp"
#include "Base/Renderer/Camera.hpp"
#include "Base/Renderer/Pipeline.hpp"
#include "Base/Renderer/Shader.hpp"
#include "Base/Renderer/SharedStorageBuffer.hpp"
#endif

// Data Strutures
#ifndef TF3D_EXCLUDE_DATA_STRCUTRUES
#include "Base/DS/DS.hpp"
#include "Base/DS/Mesh.hpp"
#endif

// ImGui
#include "imgui/imgui.h"