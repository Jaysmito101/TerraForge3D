#pragma once

#ifdef TERR3D_WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

// STL Libs

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <fstream>
#include <iostream>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stack>
#include <string>
#include <thread>

#include <filesystem>

#include <fmt/core.h>

#define TF3D_FWD_DEC_CLASS(className, namespaceName) \
    namespace namespaceName                          \
    {                                                \
        class className;                             \
    }                                                \
    using namespaceName::className;

#define TF3D_FWD_DEC_STRUCT(structName, namespaceName) \
    namespace namespaceName                            \
    {                                                  \
        struct structName;                             \
    }                                                  \
    using namespaceName::structName;

// OS Dependent Libraries

// GLM

#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/scalar_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/matrix.hpp>

// IMGUI

#include <imgui/imgui.h>

// TerraForge3D Base

#include "AsyncTextureReadback.h"
#include "Application.h"
#include "Camera.h"
#include "DataTexture.h"
#include "EventManager.h"
#include "ExportTexture.h"
#include "FrameBuffer.h"
#include "ImGuiCurveEditor.h"
#include "ImGuiShapes.h"
#include "Mesh.h"
#include "Model.h"
#include "ModelImporter.h"
#include "Renderer.h"
#include "Shader.h"
#include "ShaderStorageBuffer.h"
#include "Texture2D.h"
#include "Texture2DStorage.h"
#include "TextureCubemap.h"
#include "UIFontManager.h"
#include "Window.h"

// GLFW

#include <GLFW/glfw3.h>
#include <glad/gl.h>

// Macros
