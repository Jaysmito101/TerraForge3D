#pragma once

#ifdef TERR3D_WIN32
#include <Windows.h>
#endif


// STL Libs

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>

#include <iostream>
#include <fstream>

#include <array>
#include <algorithm>
#include <stack>
#include <queue>
#include <memory>
#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <filesystem>


#include <fmt/core.h>


// OS Dependent Libraries

// GLM

#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>
#include <glm/gtc/type_ptr.hpp>


// IMGUI

#include <imgui/imgui.h>

// TerraForge3D Base

#include "Window.h"
#include "Application.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Model.h"
#include "Texture2D.h"
#include "DataTexture.h"
#include "ExportTexture.h"
#include "FrameBuffer.h"
#include "ImGuiCurveEditor.h"
#include "ImGuiShapes.h"
#include "ModelImporter.h"
#include "Renderer.h"
#include "ShaderStorageBuffer.h"
#include "ComputeShader.h"
#include "TextureCubemap.h"
#include "UIFontManager.h"
#include "EventManager.h"

// GLFW

#include <glad/glad.h>
#include <GLFW/glfw3.h>


// GLAD

#include <glad/glad.h>

// Macros



